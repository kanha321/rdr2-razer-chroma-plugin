/*
 * GameStateMachine.h — High-level game state manager
 *
 * Centralizes gameplay state with debounce logic to prevent flickering.
 * Reads raw GameState and determines high-level mode (IDLE, COMBAT, etc.)
 * Emits state transition events via EventBus.
 *
 * Combat state persists for COMBAT_DEBOUNCE_MS after last trigger
 * to avoid rapid IDLE↔COMBAT flicker during gunfights.
 */

#pragma once

#include "EventBus.h"
#include "../GameState.h"

// ============================================================================
// High-level game states
// ============================================================================
enum class HighLevelState
{
    IDLE,
    EXPLORATION,
    COMBAT,
    DEAD_EYE,
    MENU,
    CUTSCENE
};

inline const char* HighLevelStateName(HighLevelState state)
{
    switch (state)
    {
    case HighLevelState::IDLE:        return "IDLE";
    case HighLevelState::EXPLORATION: return "EXPLORATION";
    case HighLevelState::COMBAT:      return "COMBAT";
    case HighLevelState::DEAD_EYE:    return "DEAD_EYE";
    case HighLevelState::MENU:        return "MENU";
    case HighLevelState::CUTSCENE:    return "CUTSCENE";
    default:                          return "UNKNOWN";
    }
}

class GameStateMachine
{
public:
    GameStateMachine();

    // Update state from raw game data — call once per frame
    // Emits events to the EventBus on state transitions
    void Update(const GameState& state, EventBus& bus);

    // Current high-level state
    HighLevelState GetCurrentState() const { return m_currentState; }

private:
    HighLevelState m_currentState;

    // Previous raw state for edge detection
    bool m_wasWanted;
    bool m_wasLowHealth;
    bool m_wasDeadEye;
    bool m_wasPlayerValid;

    // Combat debounce
    DWORD m_lastCombatTrigger;
    static constexpr DWORD COMBAT_DEBOUNCE_MS = 5000;  // 5 seconds

    // Health threshold
    static constexpr float LOW_HEALTH_THRESHOLD = 0.25f;

    // Helper: determine if health is low
    bool IsHealthLow(const GameState& state) const;
};
