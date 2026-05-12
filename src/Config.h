/*
 * Config.h — Centralized configuration constants
 *
 * Phase 2: Updated with rendering pipeline constants, effect parameters,
 * and layer configuration. All tunable values in one place.
 */

#pragma once

namespace Config
{
    // ========================================================================
    // Chroma SDK Connection
    // ========================================================================
    constexpr const char* CHROMA_BASE_URL   = "localhost";
    constexpr int         CHROMA_PORT       = 54235;
    constexpr const char* CHROMA_ENDPOINT   = "/razer/chromasdk";

    // ========================================================================
    // App Registration
    // ========================================================================
    constexpr const char* APP_TITLE         = "RDR2ChromaSync";
    constexpr const char* APP_DESCRIPTION   = "RDR2 event-driven RGB lighting";
    constexpr const char* APP_AUTHOR_NAME   = "RDR2ChromaSync";
    constexpr const char* APP_AUTHOR_URL    = "https://github.com/rdr2chromasync";
    constexpr const char* APP_CATEGORY      = "game";

    // ========================================================================
    // Timing
    // ========================================================================
    constexpr int RENDER_INTERVAL_MS          = 33;     // ~30 FPS (Chroma SDK max)
    constexpr int HEARTBEAT_INTERVAL_MS       = 1000;   // Chroma heartbeat
    constexpr int CHROMA_INIT_RETRY_INTERVAL_MS = 5000; // Retry init every 5s
    constexpr int WARMUP_FRAMES               = 30;     // Delay before HTTP calls

    // ========================================================================
    // Debug Logging Intervals
    // ========================================================================
    constexpr int DEBUG_LOG_INTERVAL_MS = 5000;  // Log debug stats every 5 seconds

    // ========================================================================
    // Game State Machine
    // ========================================================================
    constexpr float LOW_HEALTH_THRESHOLD = 0.25f;  // 25% of max health
    constexpr int   COMBAT_DEBOUNCE_MS   = 5000;   // Stay in combat 5s after last trigger

    // ========================================================================
    // Effect Parameters
    // ========================================================================

    // Pulse effect (for wanted level)
    constexpr float PULSE_FREQUENCY_HZ      = 2.0f;    // Pulses per second
    constexpr float PULSE_MIN_BRIGHTNESS    = 0.3f;    // Dimmest point

    // Breathing effect (for Dead Eye)
    constexpr float BREATHING_CYCLE_SECONDS = 3.0f;    // One full in-out cycle
    constexpr float BREATHING_MIN_BRIGHTNESS= 0.4f;    // Dimmest point

    // Flash effect (for hits, transitions)
    constexpr float FLASH_DURATION_SECONDS  = 0.3f;    // Total flash time

    // Transition effect
    constexpr float TRANSITION_DURATION_SECONDS = 0.5f; // Color interpolation time

    // ========================================================================
    // Keyboard Grid (Chroma CUSTOM)
    // ========================================================================
    constexpr int KEYBOARD_ROWS = 6;
    constexpr int KEYBOARD_COLS = 22;

    // ========================================================================
    // Phase 1 backward-compatible BGR colors
    // (kept for reference — Phase 2 uses Color struct instead)
    // ========================================================================
    constexpr int COLOR_WANTED_BGR    = 0x0000FF;  // Red
    constexpr int COLOR_LOW_HP_BGR    = 0x0080FF;  // Orange
    constexpr int COLOR_DEAD_EYE_BGR  = 0x2D5580;  // Sepia
    constexpr int COLOR_IDLE_BGR      = 0x404040;  // Dim white

    // Backward-compatible aliases (used by Phase 1 EventDetector.cpp)
    constexpr int COLOR_WANTED     = COLOR_WANTED_BGR;
    constexpr int COLOR_LOW_HEALTH = COLOR_LOW_HP_BGR;
    constexpr int COLOR_DEAD_EYE   = COLOR_DEAD_EYE_BGR;
    constexpr int COLOR_NONE       = COLOR_IDLE_BGR;
}
