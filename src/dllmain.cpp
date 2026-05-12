/*
 * dllmain.cpp — ASI entry point and Phase 2 render pipeline
 *
 * Replaces the Phase 1 direct event→color loop with a full rendering pipeline:
 *   game state → event bus → state machine → effects → layers → compositor → renderer → Chroma SDK
 *
 * CRITICAL PATTERNS (proven in Phase 1):
 *   - Warmup delay before HTTP calls (prevents game freeze)
 *   - WAIT() must be called every iteration (ScriptHook fiber requirement)
 *   - Try/catch around all external calls (stability)
 *   - Graceful Chroma session recovery on failure
 *
 * Reference: Phase 1 dllmain.cpp — warmup, WAIT(), try/catch pattern
 * Reference: RDR2Extension/main.cpp — scriptRegister lifecycle
 */

#include "main.h"
#include "types.h"
#include "Config.h"
#include "GameState.h"
#include "ChromaClient.h"
#include "Logger.h"

// Phase 2 systems
#include "events/EventBus.h"
#include "events/EventTypes.h"
#include "events/GameStateMachine.h"
#include "effects/EffectRegistry.h"
#include "effects/StaticEffect.h"
#include "effects/PulseEffect.h"
#include "effects/FlashEffect.h"
#include "effects/BreathingEffect.h"
#include "effects/TransitionEffect.h"
#include "effects/RippleEffect.h"
#include "render/LayerStack.h"
#include "render/Compositor.h"
#include "render/ChromaRenderer.h"
#include "core/Color.h"
#include "core/Framebuffer.h"
#include "JsonConfig.h"

// ============================================================================
// Globals
// ============================================================================
static ChromaClient* g_chroma = nullptr;
static HMODULE g_hModule = nullptr;

// ============================================================================
// Register default event→effect mappings
// ============================================================================
static void RegisterEffects(EffectRegistry& registry)
{
    auto& cfg = JsonConfig::Instance();
    LOG_INFO("RegisterEffects: Setting up event→effect mappings");

    // ── BASE layer: idle/default state (subtle breathing so keyboard feels alive) ──
    registry.Register(EventType::STATE_IDLE, "BASE", [&cfg]() -> Effect* {
        return new BreathingEffect(cfg.colorIdle, 5.0f, Easing::Type::SINE, 0.5f, 0);
    });

    // ── COMBAT layer: wanted level ─────────────────────────────────────
    registry.Register(EventType::WANTED_STARTED, "COMBAT", [&cfg]() -> Effect* {
        return new PulseEffect(
            cfg.colorWanted,
            cfg.pulseFrequencyHz,
            cfg.pulseMinBrightness,
            30  // priority
        );
    });

    registry.Register(EventType::WANTED_CLEARED, "COMBAT", nullptr);  // Clear layer

    // ── CRITICAL layer: Dead Eye ───────────────────────────────────────
    registry.Register(EventType::DEAD_EYE_ACTIVATED, "CRITICAL", [&cfg]() -> Effect* {
        return new BreathingEffect(
            cfg.colorDeadEye,
            cfg.breathingCycleSeconds,
            Easing::Type::SINE,
            cfg.breathingMinBrightness,
            40  // priority
        );
    });

    registry.Register(EventType::DEAD_EYE_DEACTIVATED, "CRITICAL", nullptr);

    // ── COMBAT layer (also): low health ────────────────────────────────
    registry.Register(EventType::LOW_HEALTH_ENTERED, "COMBAT", [&cfg]() -> Effect* {
        return new PulseEffect(
            cfg.colorLowHealth,
            cfg.pulseFrequencyHz * 1.5f,  // Faster pulse for urgency
            cfg.pulseMinBrightness,
            35  // Slightly higher priority than wanted
        );
    });

    registry.Register(EventType::LOW_HEALTH_EXITED, "COMBAT", nullptr);

    LOG_INFO("RegisterEffects: All mappings registered");
}

// ============================================================================
// ScriptMain — Runs on the ScriptHook fiber thread
// ============================================================================
void ScriptMain()
{
    LOG_INFO("ScriptMain: ===== Phase 2 Render Pipeline Starting =====");
    auto& cfg = JsonConfig::Instance();
    // ────────────────────────────────────────────────────────────────────
    // 1. Create systems
    // ────────────────────────────────────────────────────────────────────
    g_chroma = new ChromaClient();

    EventBus eventBus;
    GameStateMachine stateMachine;
    LayerStack layers;
    EffectRegistry registry;
    Framebuffer finalBuffer;

    // Register event→effect mappings
    RegisterEffects(registry);
    registry.BindToEventBus(eventBus, layers);

    LOG_INFO("ScriptMain: All systems initialized");

    // ────────────────────────────────────────────────────────────────────
    // 2. Warmup delay (proven pattern from Phase 1)
    // ────────────────────────────────────────────────────────────────────
    LOG_INFO("ScriptMain: Starting warmup (" + std::to_string(cfg.warmupFrames) + " frames)...");
    for (int i = 0; i < cfg.warmupFrames; i++)
    {
        WAIT(cfg.renderIntervalMs);
    }
    LOG_INFO("ScriptMain: Warmup complete");

    // ────────────────────────────────────────────────────────────────────
    // 3. Renderer (needs ChromaClient, created after warmup)
    // ────────────────────────────────────────────────────────────────────
    ChromaRenderer* renderer = nullptr;

    // Delta time tracking (using real wall clock, not game time)
    ULONGLONG lastFrameTime = GetTickCount64();
    int frameCount = 0;
    DWORD lastDebugLogTime = GetTickCount();

    // Set idle effect on BASE layer at startup
    {
        Layer* baseLayer = layers.GetLayer("BASE");
        if (baseLayer)
        {
            Effect* idleEffect = new BreathingEffect(cfg.colorIdle, 5.0f, Easing::Type::SINE, 0.5f, 0);
            idleEffect->Start();
            baseLayer->activeEffect = idleEffect;
            LOG_INFO("ScriptMain: Initial idle BreathingEffect set on BASE layer");
        }
    }

    LOG_INFO("ScriptMain: Entering main render loop");

    // ════════════════════════════════════════════════════════════════════
    // MAIN RENDER LOOP
    // ════════════════════════════════════════════════════════════════════
    while (true)
    {
        // ── Calculate delta time (real time, NOT game time) ─────────────
        ULONGLONG now = GetTickCount64();
        float deltaTime = static_cast<float>(now - lastFrameTime) / 1000.0f;
        lastFrameTime = now;

        // Clamp delta time to prevent explosion after breakpoints/pauses
        if (deltaTime > 0.5f) deltaTime = 0.033f;

        frameCount++;

        // ── Step 1: Ensure Chroma session ──────────────────────────────
        if (!g_chroma->IsReady())
        {
            try
            {
                g_chroma->Initialize();
            }
            catch (const std::exception& e)
            {
                LOG_ERROR("ScriptMain: Exception in Initialize: " + std::string(e.what()));
            }
            catch (...)
            {
                LOG_ERROR("ScriptMain: Unknown exception in Initialize");
            }

            if (g_chroma->IsReady())
            {
                LOG_INFO("ScriptMain: Chroma session established!");
                // Create renderer now that we have a connection
                if (renderer)
                    delete renderer;
                renderer = new ChromaRenderer(*g_chroma);
            }
        }

        // ── Step 2: Heartbeat ──────────────────────────────────────────
        if (g_chroma->IsReady())
        {
            try { g_chroma->Heartbeat(); }
            catch (...) { LOG_ERROR("ScriptMain: Exception in Heartbeat"); }
        }

        // ── Step 3: Read game state ────────────────────────────────────
        GameState state;
        try
        {
            state = GameState::Read();
        }
        catch (...)
        {
            LOG_ERROR("ScriptMain: Exception in GameState::Read");
            state.isPlayerValid = false;
        }

        // ── Step 4: Update state machine → emits events to bus ─────────
        try
        {
            stateMachine.Update(state, eventBus);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("ScriptMain: Exception in StateMachine: " + std::string(e.what()));
        }
        catch (...)
        {
            LOG_ERROR("ScriptMain: Unknown exception in StateMachine");
        }

        // ── Step 5: Process event queue ────────────────────────────────
        // EventBus dispatches to all subscribers including the EffectRegistry
        // (which subscribed via SubscribeAll in BindToEventBus)
        try
        {
            eventBus.ProcessQueue();
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("ScriptMain: Exception in ProcessQueue: " + std::string(e.what()));
        }
        catch (...)
        {
            LOG_ERROR("ScriptMain: Unknown exception in ProcessQueue");
        }

        // ── Step 6: Update effects on all layers ───────────────────────
        try
        {
            layers.UpdateEffects(deltaTime);
        }
        catch (...)
        {
            LOG_ERROR("ScriptMain: Exception in UpdateEffects");
        }

        // ── Step 7: Render effects into layer framebuffers ─────────────
        try
        {
            layers.RenderEffects();
        }
        catch (...)
        {
            LOG_ERROR("ScriptMain: Exception in RenderEffects");
        }

        // ── Step 8: Composite layers into final framebuffer ────────────
        Compositor compositor;
        try
        {
            compositor.Composite(layers, finalBuffer);
        }
        catch (...)
        {
            LOG_ERROR("ScriptMain: Exception in Composite");
        }

        // ── Step 9: Push to hardware ───────────────────────────────────
        if (renderer && g_chroma->IsReady())
        {
            try
            {
                renderer->Render(finalBuffer);
            }
            catch (...)
            {
                LOG_ERROR("ScriptMain: Exception in Render");
            }
        }

        // ── Step 10: Periodic debug logging ────────────────────────────
        DWORD nowMs = GetTickCount();
        if ((nowMs - lastDebugLogTime) >= (DWORD)cfg.debugLogIntervalMs)
        {
            LOG_DEBUG("ScriptMain: [STATS] frame=" + std::to_string(frameCount) +
                      " dt=" + std::to_string(deltaTime) +
                      " activeLayers=" + std::to_string(layers.ActiveLayerCount()) +
                      " state=" + std::string(HighLevelStateName(stateMachine.GetCurrentState())) +
                      " chromaReady=" + std::string(g_chroma->IsReady() ? "yes" : "no"));

            if (renderer)
            {
                LOG_DEBUG("ScriptMain: [RENDER] sent=" + std::to_string(renderer->GetFramesSent()) +
                          " skipped=" + std::to_string(renderer->GetFramesSkipped()));
            }

            lastDebugLogTime = nowMs;
        }

        // ── Step 11: Yield to game engine ──────────────────────────────
        // MUST ALWAYS REACH THIS — ScriptHook fiber requirement
        WAIT(cfg.renderIntervalMs);
    }

    // ════════════════════════════════════════════════════════════════════
    // Cleanup (reached if ScriptHook terminates the fiber)
    // ════════════════════════════════════════════════════════════════════
    layers.Cleanup();

    if (renderer)
    {
        delete renderer;
        renderer = nullptr;
    }

    if (g_chroma)
    {
        g_chroma->Shutdown();
        delete g_chroma;
        g_chroma = nullptr;
    }

    Logger::Instance().Close();
}

// ============================================================================
// DllMain — ASI Entry Point
// ============================================================================
BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        g_hModule = hInstance;
        Logger::Instance().Init(hInstance);
        LOG_INFO("DllMain: DLL_PROCESS_ATTACH (Phase 2)");

        // Load JSON config before anything else
        JsonConfig::Instance().Load(hInstance);

        scriptRegister(hInstance, ScriptMain);
        LOG_INFO("DllMain: scriptRegister called");
        break;

    case DLL_PROCESS_DETACH:
        LOG_INFO("DllMain: DLL_PROCESS_DETACH");

        if (g_chroma)
        {
            g_chroma->Shutdown();
            delete g_chroma;
            g_chroma = nullptr;
        }

        scriptUnregister(hInstance);
        Logger::Instance().Close();
        break;
    }

    return TRUE;
}
