/*
 * Config.h — Constants and thresholds for RDR2ChromaSync
 *
 * All tunable values are defined here for easy adjustment.
 * Color values use BGR format as required by the Razer Chroma REST API.
 * BGR formula: (Blue << 16) | (Green << 8) | Red
 * Ref: RGB4R/Extensions/ColorExtensions.cs — confirmed production BGR formula
 */

#pragma once

namespace Config
{
    // ========================================================================
    // Razer Chroma REST API
    // ========================================================================

    // Base URL for the Chroma SDK REST server (Razer Synapse 3)
    constexpr const char* CHROMA_BASE_URL = "localhost";
    constexpr int CHROMA_PORT = 54235;
    constexpr const char* CHROMA_ENDPOINT = "/razer/chromasdk";

    // Application registration payload fields
    constexpr const char* APP_TITLE       = "RDR2ChromaSync";
    constexpr const char* APP_DESCRIPTION = "RDR2 event-driven RGB lighting";
    constexpr const char* APP_AUTHOR_NAME = "RDR2ChromaSync";
    constexpr const char* APP_AUTHOR_URL  = "https://github.com/rdr2chromasync";
    constexpr const char* APP_CATEGORY    = "game";

    // ========================================================================
    // Timing
    // ========================================================================

    // Main loop polling interval (ms)
    // ScriptHookRDR2's WAIT() granularity
    constexpr int POLL_INTERVAL_MS = 100;

    // Chroma heartbeat interval (ms)
    // The Chroma SDK server times out after 15 seconds.
    // RGB4R sends heartbeat every 1 second — we match that proven interval.
    // Ref: RGB4R/Chroma.cs:L76 — "lastHeartbeat + 1000 <= GameTime"
    constexpr int HEARTBEAT_INTERVAL_MS = 1000;

    // Max retries for Chroma initialization before giving up for this cycle
    constexpr int CHROMA_INIT_RETRY_INTERVAL_MS = 5000;

    // ========================================================================
    // Event Colors (BGR format)
    // ========================================================================
    // Razer Chroma uses BGR, NOT RGB.
    // Formula: color = (B << 16) | (G << 8) | R
    // Ref: RGB4R/Extensions/ColorExtensions.cs:L17

    // EVENT_WANTED — Red (R=255, G=0, B=0)
    // BGR: (0 << 16) | (0 << 8) | 255 = 0x0000FF
    constexpr int COLOR_WANTED = 0x0000FF;

    // EVENT_LOW_HEALTH — Orange (R=255, G=128, B=0)
    // BGR: (0 << 16) | (128 << 8) | 255 = 0x0080FF
    constexpr int COLOR_LOW_HEALTH = 0x0080FF;

    // EVENT_DEAD_EYE — Sepia/Amber (R=128, G=85, B=45)
    // BGR: (45 << 16) | (85 << 8) | 128 = 0x2D5580
    constexpr int COLOR_DEAD_EYE = 0x2D5580;

    // EVENT_NONE — Dim White (R=64, G=64, B=64)
    // BGR: (64 << 16) | (64 << 8) | 64 = 0x404040
    constexpr int COLOR_NONE = 0x404040;

    // ========================================================================
    // Health Threshold
    // ========================================================================

    // Low health triggers when (currentHealth - 100) / (maxHealth - 100) < this value
    // RDR2 health range: 100 (dead) to maxHealth (typically 200)
    // So effective health = currentHealth - 100, effective max = maxHealth - 100
    constexpr float LOW_HEALTH_THRESHOLD = 0.25f;
}
