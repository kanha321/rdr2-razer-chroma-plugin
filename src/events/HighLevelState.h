/*
 * HighLevelState.h — High-level game state definitions
 *
 * Shared enum and name helpers so multiple systems can depend on state
 * without coupling to GameStateMachine's concrete implementation.
 */

#pragma once

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
