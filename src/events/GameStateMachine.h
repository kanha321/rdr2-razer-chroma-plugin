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

#include "IStateMachine.h"

class GameStateMachine : public IStateMachine
{
public:
    GameStateMachine();

    // Update state from raw game data — call once per frame
    // Emits events to the EventBus on state transitions
    void Update(const GameState& state, IEventBus& bus) override;

    // Current high-level state
    HighLevelState GetCurrentState() const override { return m_currentState; }

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
