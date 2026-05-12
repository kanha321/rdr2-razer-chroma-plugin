/*
 * EventDetector.cpp — Determines active event from GameState
 *
 * Priority logic: Dead Eye > Wanted > Low Health > None
 *
 * The low health formula accounts for RDR2's health range:
 *   - Health 100 = dead, maxHealth typically 200
 *   - Effective health ratio = (current - 100) / (max - 100)
 *   - Triggers when ratio < 0.25 (25%)
 */

#include "EventDetector.h"
#include "Config.h"

GameEvent EventDetector::Detect(const GameState& state)
{
    // If player isn't valid (loading, cutscene, etc.), return no event
    if (!state.isPlayerValid)
    {
        return GameEvent::EVENT_NONE;
    }

    // Priority 1 (highest): Dead Eye active
    // Dead Eye is the most time-critical state — player is actively
    // in slow-motion aiming. This overrides everything.
    if (state.isDeadEyeActive)
    {
        return GameEvent::EVENT_DEAD_EYE;
    }

    // Priority 2: Wanted level
    // Any wanted level >= 1 star triggers this event.
    // Pattern from RGB4R/Script.cs:L92 — "if (wanted > 0)"
    if (state.wantedLevel > 0)
    {
        return GameEvent::EVENT_WANTED;
    }

    // Priority 3: Low health
    // Formula from PRD: (currentHealth - 100) / (maxHealth - 100) < 0.25
    // Guard against division by zero (shouldn't happen, but defensive)
    int effectiveMax = state.maxHealth - 100;
    if (effectiveMax > 0)
    {
        float healthRatio = static_cast<float>(state.currentHealth - 100) / static_cast<float>(effectiveMax);
        if (healthRatio < Config::LOW_HEALTH_THRESHOLD)
        {
            return GameEvent::EVENT_LOW_HEALTH;
        }
    }

    // Priority 4 (lowest): No active event — idle state
    return GameEvent::EVENT_NONE;
}

int EventDetector::GetColor(GameEvent event)
{
    switch (event)
    {
    case GameEvent::EVENT_DEAD_EYE:   return Config::COLOR_DEAD_EYE;
    case GameEvent::EVENT_WANTED:     return Config::COLOR_WANTED;
    case GameEvent::EVENT_LOW_HEALTH: return Config::COLOR_LOW_HEALTH;
    case GameEvent::EVENT_NONE:
    default:                          return Config::COLOR_NONE;
    }
}

const char* EventDetector::GetName(GameEvent event)
{
    switch (event)
    {
    case GameEvent::EVENT_DEAD_EYE:   return "DEAD_EYE";
    case GameEvent::EVENT_WANTED:     return "WANTED";
    case GameEvent::EVENT_LOW_HEALTH: return "LOW_HEALTH";
    case GameEvent::EVENT_NONE:
    default:                          return "NONE";
    }
}
