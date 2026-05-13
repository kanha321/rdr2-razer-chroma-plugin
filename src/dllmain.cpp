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
#include "effects/VignetteEffect.h"
#include "effects/WaveEffect.h"
#include "render/LayerStack.h"
#include "render/Compositor.h"
#include "render/ChromaRenderer.h"
#include "core/Color.h"
#include "core/Framebuffer.h"
#include "JsonConfig.h"
#include "runtime/DefaultGameStateReader.h"
#include "runtime/IFrameRenderer.h"

// Phase 3 systems
#include "transitions/TransitionManager.h"
#include "transitions/TransitionPolicy.h"
#include "animation/TimelineComposer.h"

// ============================================================================
// Globals
// ============================================================================
static ChromaClient* g_chroma = nullptr;
static HMODULE g_hModule = nullptr;

// ============================================================================
// Register default event→effect mappings (Phase 3: choreographed)
// ============================================================================
static TransitionManager* g_transitionMgr = nullptr;
static TimelineComposer*  g_timeline = nullptr;
static LayerStack*        g_layers = nullptr;

static void RegisterEffects(EffectRegistry& registry)
{
    auto& cfg = JsonConfig::Instance();
    LOG_INFO("RegisterEffects: Setting up Phase 3 choreographed mappings");

    // ── BASE layer: idle breathing ──────────────────────────────────────
    registry.Register(EventType::STATE_IDLE, "BASE", [&cfg]() -> Effect* {
        LOG_INFO("[P3-EFFECT] Creating BreathingEffect for IDLE on BASE (cycle=5s, minBr=0.5)");
        return new BreathingEffect(cfg.colorIdle, 5.0f, Easing::Type::SINE, 0.5f, 0);
    });

    // ── COMBAT layer: wanted level ─────────────────────────────────────
    // Phase 3: On wanted start, fire a red ripple on AMBIENT, then
    // crossfade COMBAT to pulsing red — choreographed via timeline
    registry.Register(EventType::WANTED_STARTED, "COMBAT", [&cfg]() -> Effect* {
        // Schedule a ripple burst on FX layer (above COMBAT, ADD blend)
        if (g_timeline && g_layers && g_transitionMgr)
        {
            LOG_INFO("[P3-CHOREOGRAPHY] Wanted started — scheduling ripple burst on FX (50ms delay)");
            g_timeline->Schedule(0.05f, [&cfg]() {
                Layer* fxLayer = g_layers->GetLayer("FX");
                if (fxLayer)
                {
                    LOG_INFO("[P3-CHOREOGRAPHY] Firing RippleEffect on FX (speed=20, width=4)");
                    auto* ripple = new RippleEffect(
                        cfg.colorWanted, 20.0f, 4.0f, Color::Black(), 15);
                    g_transitionMgr->RequestTransition(fxLayer, ripple);
                }
            }, "combat_ripple_burst");
        }

        LOG_INFO("[P3-EFFECT] Creating PulseEffect for WANTED on COMBAT (freq=" +
                 std::to_string(cfg.pulseFrequencyHz) + "Hz, minBr=" +
                 std::to_string(cfg.pulseMinBrightness) + ")");
        return new PulseEffect(
            cfg.colorWanted,
            cfg.pulseFrequencyHz,
            cfg.pulseMinBrightness,
            30
        );
    });

    registry.Register(EventType::WANTED_CLEARED, "COMBAT", nullptr);

    // ── CRITICAL layer: Dead Eye (simple static color + left-to-right sweep)
    registry.Register(EventType::DEAD_EYE_ACTIVATED, "CRITICAL", [&cfg]() -> Effect* {
        // Fire a single left-to-right sweep on FX to visualize the change
        if (g_timeline && g_layers && g_transitionMgr)
        {
            LOG_INFO("[P3-CHOREOGRAPHY] Dead Eye activated — scheduling WaveEffect sweep on FX");
            g_timeline->Schedule(0.0f, [&cfg]() {
                Layer* fxLayer = g_layers->GetLayer("FX");
                if (fxLayer)
                {
                    LOG_INFO("[P3-CHOREOGRAPHY] Firing WaveEffect on FX (speed=18, trail=8, one-shot)");
                    auto* wave = new WaveEffect(
                        cfg.colorDeadEye, 18.0f, 8.0f, Color::Black(), false, 40, false);
                    g_transitionMgr->RequestTransition(fxLayer, wave);
                }
            }, "dead_eye_sweep");
        }

        LOG_INFO("[P3-EFFECT] Creating StaticEffect for DEAD_EYE on CRITICAL");
        return new StaticEffect(cfg.colorDeadEye, 40);
    });

    registry.Register(EventType::DEAD_EYE_DEACTIVATED, "CRITICAL", nullptr);

    // ── COMBAT layer: low health (Phase 3: faster pulse + red wave sweep)
    registry.Register(EventType::LOW_HEALTH_ENTERED, "COMBAT", [&cfg]() -> Effect* {
        // Schedule a warning wave sweep on FX layer (above COMBAT, ADD blend)
        if (g_timeline && g_layers && g_transitionMgr)
        {
            LOG_INFO("[P3-CHOREOGRAPHY] Low health — scheduling WaveEffect on FX (100ms delay)");
            g_timeline->Schedule(0.1f, [&cfg]() {
                Layer* fxLayer = g_layers->GetLayer("FX");
                if (fxLayer)
                {
                    LOG_INFO("[P3-CHOREOGRAPHY] Firing WaveEffect on FX (speed=12, trail=5)");
                    auto* wave = new WaveEffect(
                        cfg.colorLowHealth, 12.0f, 5.0f, Color::Black(), false, 12);
                    g_transitionMgr->RequestTransition(fxLayer, wave);
                }
            }, "low_health_wave");
        }

        LOG_INFO("[P3-EFFECT] Creating PulseEffect for LOW_HEALTH on COMBAT (freq=" +
                 std::to_string(cfg.pulseFrequencyHz * 1.5f) + "Hz)");
        return new PulseEffect(
            cfg.colorLowHealth,
            cfg.pulseFrequencyHz * 1.5f,
            cfg.pulseMinBrightness,
            35
        );
    });

    // When low health exits, clear COMBAT pulse AND FX wave
    registry.Register(EventType::LOW_HEALTH_EXITED, "COMBAT", nullptr);
    registry.Register(EventType::LOW_HEALTH_EXITED, "FX", nullptr);

    // When wanted clears, also clear any combat ripple on FX
    registry.Register(EventType::WANTED_CLEARED, "FX", nullptr);

    // When Dead Eye ends, clear any FX that might have been scheduled
    registry.Register(EventType::DEAD_EYE_DEACTIVATED, "FX", nullptr);

    // ── Clean up all transient layers when returning to idle ─────────────
    // This is the catch-all: any orphaned FX/AMBIENT effects get cleaned
    registry.Register(EventType::STATE_IDLE, "FX", nullptr);
    registry.Register(EventType::STATE_IDLE, "AMBIENT", nullptr);
    registry.Register(EventType::STATE_IDLE, "COMBAT", nullptr);

    LOG_INFO("RegisterEffects: All Phase 3 mappings registered");
}

// ============================================================================
// ScriptMain — Runs on the ScriptHook fiber thread
// ============================================================================
void ScriptMain()
{
    LOG_INFO("ScriptMain: ===== Phase 3 Render Pipeline Starting =====");
    auto& cfg = JsonConfig::Instance();
    // ────────────────────────────────────────────────────────────────────
    // 1. Create systems
    // ────────────────────────────────────────────────────────────────────
    g_chroma = new ChromaClient();

    EventBus eventBus;
    GameStateMachine stateMachine;
    DefaultGameStateReader gameStateReader;
    LayerStack layers;
    EffectRegistry registry;
    TransitionManager transitionMgr;
    TimelineComposer timeline;
    Framebuffer finalBuffer;

    // ── Setup transition policies from config ────────────────────────────
    transitionMgr.SetPolicy("BASE",
        { cfg.baseFadeInSec, cfg.baseFadeOutSec, Easing::Type::SINE, true, false });
    transitionMgr.SetPolicy("COMBAT",
        { cfg.combatFadeInSec, cfg.combatFadeOutSec, Easing::Type::EASE_IN, true, false });
    transitionMgr.SetPolicy("CRITICAL",
        { cfg.criticalFadeInSec, cfg.criticalFadeOutSec, Easing::Type::SINE, true, false });
    transitionMgr.SetPolicy("AMBIENT",
        TransitionPolicy::Cinematic(1.0f, 1.0f));
    transitionMgr.SetPolicy("FX",
        { 0.05f, 0.3f, Easing::Type::LINEAR, false, false });  // Near-instant in, fast out
    transitionMgr.SetPolicy("WEATHER",
        TransitionPolicy::Cinematic(2.0f, 2.0f));

    // Wire up global pointers for timeline choreography in RegisterEffects
    g_transitionMgr = &transitionMgr;
    g_timeline = &timeline;
    g_layers = &layers;

    // Register event→effect mappings and bind via TransitionManager
    RegisterEffects(registry);
    registry.BindToEventBus(eventBus, layers, transitionMgr);

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
    IFrameRenderer* renderer = nullptr;

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
            state = gameStateReader.Read();
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
        // EventBus dispatches to EffectRegistry, which routes through TransitionManager
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

        // ── Step 5.5: Update timeline composer (Phase 3) ──────────────
        timeline.Update(deltaTime);

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

        // ── Step 10: Periodic debug logging (Phase 3: enhanced) ─────────
        DWORD nowMs = GetTickCount();
        if ((nowMs - lastDebugLogTime) >= (DWORD)cfg.debugLogIntervalMs)
        {
            LOG_DEBUG("ScriptMain: [STATS] frame=" + std::to_string(frameCount) +
                      " dt=" + std::to_string(deltaTime) +
                      " activeLayers=" + std::to_string(layers.ActiveLayerCount()) +
                      " state=" + std::string(HighLevelStateName(stateMachine.GetCurrentState())) +
                      " chromaReady=" + std::string(g_chroma->IsReady() ? "yes" : "no") +
                      " timelinePending=" + std::to_string(timeline.PendingCount()));

            // Phase 3: Log per-layer state (opacity, effect names, outgoing)
            auto sortedLayers = layers.GetSortedLayers();
            for (Layer* layer : sortedLayers)
            {
                if (layer->HasContent() || layer->outgoingEffect || layer->opacity < 1.0f)
                {
                    std::string effectName = layer->activeEffect
                        ? layer->activeEffect->GetName() : "none";
                    std::string outName = layer->outgoingEffect
                        ? layer->outgoingEffect->GetName() : "none";
                    float effectOpacity = layer->activeEffect
                        ? layer->activeEffect->opacity : 0.0f;
                    float outOpacity = layer->outgoingEffect
                        ? layer->outgoingEffect->opacity : 0.0f;

                    LOG_DEBUG("  [LAYER] " + layer->name +
                              " | opacity=" + std::to_string(layer->opacity).substr(0,4) +
                              " | effect=" + effectName +
                              "(" + std::to_string(effectOpacity).substr(0,4) + ")" +
                              " | outgoing=" + outName +
                              "(" + std::to_string(outOpacity).substr(0,4) + ")");
                }
            }

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
