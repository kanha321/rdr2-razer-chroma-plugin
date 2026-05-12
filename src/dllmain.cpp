/*
 * dllmain.cpp — ASI entry point and main loop for RDR2ChromaSync
 *
 * This is the core plugin file that ties everything together:
 *   1. DllMain registers/unregisters the script with ScriptHookRDR2
 *   2. ScriptMain runs the main loop on the ScriptHook thread
 *
 * CRITICAL: Chroma initialization is DEFERRED until after a few
 * WAIT() cycles to let the game fully load. This prevents the
 * synchronous HTTP call from freezing the game during script startup.
 */

#include "main.h"
#include "types.h"
#include "Config.h"
#include "GameState.h"
#include "EventDetector.h"
#include "ChromaClient.h"
#include "Logger.h"

// Global Chroma client instance
static ChromaClient* g_chroma = nullptr;

// Global module handle for scriptUnregister
static HMODULE g_hModule = nullptr;

// ============================================================================
// ScriptMain — Runs on the ScriptHook thread
// ============================================================================

void ScriptMain()
{
    LOG_INFO("ScriptMain: Entry point reached");

    // Create the Chroma client (only creates WinHTTP session, no network calls)
    g_chroma = new ChromaClient();

    // Track the last event to detect changes
    GameEvent lastEvent = GameEvent::EVENT_NONE;
    bool lastEventSent = false;

    // CRITICAL FIX: Defer Chroma initialization for a few frames.
    // The game needs time to fully load. Making HTTP calls too early on
    // the ScriptHook thread can block/freeze the game engine.
    // We let a few WAIT() cycles pass first.
    int warmupFrames = 30;  // ~3 seconds at 100ms per frame
    LOG_INFO("ScriptMain: Starting warmup (" + std::to_string(warmupFrames) + " frames)...");

    for (int i = 0; i < warmupFrames; i++)
    {
        WAIT(Config::POLL_INTERVAL_MS);
    }

    LOG_INFO("ScriptMain: Warmup complete, entering main loop");

    // ========================================================================
    // Main Loop
    // ========================================================================

    while (true)
    {
        // ----------------------------------------------------------------
        // Step 1: Ensure Chroma session is alive
        // ----------------------------------------------------------------
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

            // If we just connected, force-send the current event color
            if (g_chroma->IsReady())
            {
                lastEventSent = false;
                LOG_INFO("ScriptMain: Chroma session established!");
            }
        }

        // ----------------------------------------------------------------
        // Step 2: Send heartbeat (time-guarded)
        // ----------------------------------------------------------------
        if (g_chroma->IsReady())
        {
            try
            {
                g_chroma->Heartbeat();
            }
            catch (const std::exception& e)
            {
                LOG_ERROR("ScriptMain: Exception in Heartbeat: " + std::string(e.what()));
            }
            catch (...)
            {
                LOG_ERROR("ScriptMain: Unknown exception in Heartbeat");
            }
        }

        // ----------------------------------------------------------------
        // Step 3: Read game state
        // ----------------------------------------------------------------
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

        // ----------------------------------------------------------------
        // Step 4: Detect active event
        // ----------------------------------------------------------------
        GameEvent currentEvent = EventDetector::Detect(state);

        // ----------------------------------------------------------------
        // Step 5: Update Chroma if event changed
        // ----------------------------------------------------------------
        if (currentEvent != lastEvent || !lastEventSent)
        {
            if (g_chroma->IsReady())
            {
                try
                {
                    int color = EventDetector::GetColor(currentEvent);
                    LOG_INFO("ScriptMain: Event changed to " + std::string(EventDetector::GetName(currentEvent)));
                    if (g_chroma->SetKeyboardColor(color))
                    {
                        lastEvent = currentEvent;
                        lastEventSent = true;
                    }
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR("ScriptMain: Exception in SetKeyboardColor: " + std::string(e.what()));
                }
                catch (...)
                {
                    LOG_ERROR("ScriptMain: Unknown exception in SetKeyboardColor");
                }
            }
        }

        // ----------------------------------------------------------------
        // Step 6: Yield to game engine — MUST ALWAYS REACH THIS
        // ----------------------------------------------------------------
        WAIT(Config::POLL_INTERVAL_MS);
    }

    // ========================================================================
    // Cleanup (reached if ScriptHook terminates the script)
    // ========================================================================
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

        // Initialize logger FIRST — before anything else
        Logger::Instance().Init(hInstance);
        LOG_INFO("DllMain: DLL_PROCESS_ATTACH");

        // Register our ScriptMain function with ScriptHookRDR2
        scriptRegister(hInstance, ScriptMain);
        LOG_INFO("DllMain: scriptRegister called");
        break;

    case DLL_PROCESS_DETACH:
        LOG_INFO("DllMain: DLL_PROCESS_DETACH");

        // Clean up Chroma session before unloading
        if (g_chroma)
        {
            g_chroma->Shutdown();
            delete g_chroma;
            g_chroma = nullptr;
        }

        // Unregister our script from ScriptHookRDR2
        scriptUnregister(hInstance);

        Logger::Instance().Close();
        break;
    }

    return TRUE;
}
