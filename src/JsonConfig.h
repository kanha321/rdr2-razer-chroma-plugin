/*
 * JsonConfig.h — JSON configuration loader
 *
 * Reads config.json from the same directory as the .asi file.
 * Falls back to hardcoded defaults if the file is missing or malformed.
 * All values are loaded once at startup.
 */

#pragma once

#include <windows.h>
#include <string>
#include <fstream>
#include "core/Color.h"
#include "Logger.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

struct JsonConfig
{
    // Timing
    int renderIntervalMs      = 33;
    int heartbeatIntervalMs   = 1000;
    int initRetryIntervalMs   = 5000;
    int warmupFrames          = 30;
    int debugLogIntervalMs    = 5000;

    // Game
    float lowHealthThreshold  = 0.25f;
    int combatDebounceMs      = 5000;

    // Effects
    float pulseFrequencyHz    = 2.0f;
    float pulseMinBrightness  = 0.3f;
    float breathingCycleSeconds = 3.0f;
    float breathingMinBrightness = 0.4f;
    float flashDurationSeconds = 0.3f;
    float transitionDurationSeconds = 0.5f;

    // Colors
    Color colorIdle     = Color(64, 64, 64);
    Color colorWanted   = Color(255, 0, 0);
    Color colorLowHealth = Color(255, 128, 0);
    Color colorDeadEye  = Color(128, 85, 45);

    // Singleton access
    static JsonConfig& Instance()
    {
        static JsonConfig instance;
        return instance;
    }

    // Load from file next to the .asi
    void Load(HMODULE hModule)
    {
        // Get directory of the .asi file
        char path[MAX_PATH];
        GetModuleFileNameA(hModule, path, MAX_PATH);
        std::string dllPath(path);
        std::string dir = dllPath.substr(0, dllPath.find_last_of("\\/"));
        std::string configPath = dir + "\\RDR2ChromaSync_config.json";

        LOG_INFO("JsonConfig: Looking for config at: " + configPath);

        std::ifstream file(configPath);
        if (!file.is_open())
        {
            LOG_INFO("JsonConfig: Config file not found, using defaults");
            return;
        }

        try
        {
            json j;
            file >> j;
            file.close();

            LOG_INFO("JsonConfig: Config file loaded, parsing...");

            // Timing
            if (j.contains("timing"))
            {
                auto& t = j["timing"];
                if (t.contains("render_interval_ms"))     renderIntervalMs = t["render_interval_ms"];
                if (t.contains("heartbeat_interval_ms"))  heartbeatIntervalMs = t["heartbeat_interval_ms"];
                if (t.contains("init_retry_interval_ms")) initRetryIntervalMs = t["init_retry_interval_ms"];
                if (t.contains("warmup_frames"))          warmupFrames = t["warmup_frames"];
                if (t.contains("debug_log_interval_ms"))  debugLogIntervalMs = t["debug_log_interval_ms"];
            }

            // Game
            if (j.contains("game"))
            {
                auto& g = j["game"];
                if (g.contains("low_health_threshold"))   lowHealthThreshold = g["low_health_threshold"];
                if (g.contains("combat_debounce_ms"))     combatDebounceMs = g["combat_debounce_ms"];
            }

            // Effects
            if (j.contains("effects"))
            {
                auto& e = j["effects"];
                if (e.contains("pulse"))
                {
                    if (e["pulse"].contains("frequency_hz"))   pulseFrequencyHz = e["pulse"]["frequency_hz"];
                    if (e["pulse"].contains("min_brightness")) pulseMinBrightness = e["pulse"]["min_brightness"];
                }
                if (e.contains("breathing"))
                {
                    if (e["breathing"].contains("cycle_seconds"))   breathingCycleSeconds = e["breathing"]["cycle_seconds"];
                    if (e["breathing"].contains("min_brightness"))  breathingMinBrightness = e["breathing"]["min_brightness"];
                }
                if (e.contains("flash"))
                {
                    if (e["flash"].contains("duration_seconds"))   flashDurationSeconds = e["flash"]["duration_seconds"];
                }
                if (e.contains("transition"))
                {
                    if (e["transition"].contains("duration_seconds")) transitionDurationSeconds = e["transition"]["duration_seconds"];
                }
            }

            // Colors
            if (j.contains("colors"))
            {
                auto& c = j["colors"];
                auto readColor = [](const json& obj) -> Color {
                    return Color(
                        obj.value("r", (uint8_t)0),
                        obj.value("g", (uint8_t)0),
                        obj.value("b", (uint8_t)0)
                    );
                };

                if (c.contains("idle"))       colorIdle = readColor(c["idle"]);
                if (c.contains("wanted"))     colorWanted = readColor(c["wanted"]);
                if (c.contains("low_health")) colorLowHealth = readColor(c["low_health"]);
                if (c.contains("dead_eye"))   colorDeadEye = readColor(c["dead_eye"]);
            }

            LOG_INFO("JsonConfig: All values loaded successfully");
            LOG_INFO("  renderInterval=" + std::to_string(renderIntervalMs) + "ms");
            LOG_INFO("  pulseFreq=" + std::to_string(pulseFrequencyHz) + "Hz");
            LOG_INFO("  breathingCycle=" + std::to_string(breathingCycleSeconds) + "s");
            LOG_INFO("  combatDebounce=" + std::to_string(combatDebounceMs) + "ms");
        }
        catch (const json::exception& e)
        {
            LOG_ERROR("JsonConfig: Parse error: " + std::string(e.what()));
            LOG_INFO("JsonConfig: Falling back to defaults");
        }
    }

private:
    JsonConfig() = default;
};
