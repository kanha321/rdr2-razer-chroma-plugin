/*
 * EventDetector.h — Event enum and detection logic declarations
 *
 * Determines the active game event from a GameState snapshot
 * and maps it to the appropriate BGR color.
 *
 * Priority order (highest first):
 *   1. EVENT_DEAD_EYE  — player is in time-critical state
 *   2. EVENT_WANTED    — player has heat
 *   3. EVENT_LOW_HEALTH — player is in danger
 *   4. EVENT_NONE      — fallback / idle
 *
 * Pattern: RGB4R/Script.cs:L81-L132 — wanted level check, then character state,
 * with change detection to avoid redundant Chroma API calls.
 */

#pragma once

#include "GameState.h"

// Game events in priority order (lower enum = higher priority for clarity,
// but priority is enforced in Detect(), not by enum value)
enum class GameEvent
{
    EVENT_NONE,
    EVENT_LOW_HEALTH,
    EVENT_WANTED,
    EVENT_DEAD_EYE
};

namespace EventDetector
{
    // Evaluate the game state and return the highest-priority active event
    GameEvent Detect(const GameState& state);

    // Map a GameEvent to its BGR color value (from Config.h)
    int GetColor(GameEvent event);

    // Get a human-readable name for logging
    const char* GetName(GameEvent event);
}
