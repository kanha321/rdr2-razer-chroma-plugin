/*
 * EventTypes.h — Game event type definitions
 *
 * All event types that flow through the EventBus.
 * Split into:
 *   - State transitions (high-level game mode changes)
 *   - Trigger events (specific gameplay moments)
 */

#pragma once

#include <windows.h>
#include <string>

// ============================================================================
// Event type enum
// ============================================================================
enum class EventType
{
    // State transitions
    STATE_IDLE,
    STATE_EXPLORATION,
    STATE_COMBAT,
    STATE_DEAD_EYE,
    STATE_MENU,
    STATE_CUTSCENE,

    // Trigger events — emitted on state edges
    WANTED_STARTED,
    WANTED_CLEARED,
    LOW_HEALTH_ENTERED,
    LOW_HEALTH_EXITED,
    DEAD_EYE_ACTIVATED,
    DEAD_EYE_DEACTIVATED,

    // Count (must be last)
    _COUNT
};

// ============================================================================
// Event data payload
// ============================================================================
struct GameEventData
{
    EventType type      = EventType::STATE_IDLE;
    DWORD     timestamp = 0;     // GetTickCount() at emission time
    float     value     = 0.0f;  // Optional numeric value (e.g., wanted level, health %)
};

// ============================================================================
// Event type name lookup (for logging)
// ============================================================================
inline const char* EventTypeName(EventType type)
{
    switch (type)
    {
    case EventType::STATE_IDLE:             return "STATE_IDLE";
    case EventType::STATE_EXPLORATION:      return "STATE_EXPLORATION";
    case EventType::STATE_COMBAT:           return "STATE_COMBAT";
    case EventType::STATE_DEAD_EYE:         return "STATE_DEAD_EYE";
    case EventType::STATE_MENU:             return "STATE_MENU";
    case EventType::STATE_CUTSCENE:         return "STATE_CUTSCENE";
    case EventType::WANTED_STARTED:         return "WANTED_STARTED";
    case EventType::WANTED_CLEARED:         return "WANTED_CLEARED";
    case EventType::LOW_HEALTH_ENTERED:     return "LOW_HEALTH_ENTERED";
    case EventType::LOW_HEALTH_EXITED:      return "LOW_HEALTH_EXITED";
    case EventType::DEAD_EYE_ACTIVATED:     return "DEAD_EYE_ACTIVATED";
    case EventType::DEAD_EYE_DEACTIVATED:   return "DEAD_EYE_DEACTIVATED";
    default:                                return "UNKNOWN";
    }
}
