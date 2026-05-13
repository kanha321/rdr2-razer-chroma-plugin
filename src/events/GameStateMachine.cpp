/*
 * GameStateMachine.cpp — High-level game state manager implementation
 */

#include "GameStateMachine.h"
#include "../Logger.h"

GameStateMachine::GameStateMachine()
    : m_currentState(HighLevelState::IDLE)
    , m_wasWanted(false)
    , m_wasLowHealth(false)
    , m_wasDeadEye(false)
    , m_wasPlayerValid(false)
    , m_lastCombatTrigger(0)
{
    LOG_INFO("GameStateMachine: Initialized (state=IDLE)");
}

void GameStateMachine::Update(const GameState& state, IEventBus& bus)
{
    // Player not valid → can't determine state
    if (!state.isPlayerValid)
    {
        if (m_wasPlayerValid)
        {
            LOG_INFO("GameStateMachine: Player became invalid (menu/cutscene?)");
            m_wasPlayerValid = false;
        }
        return;
    }

    if (!m_wasPlayerValid)
    {
        LOG_INFO("GameStateMachine: Player became valid");
        m_wasPlayerValid = true;
    }

    // ====================================================================
    // Edge detection: emit trigger events on state changes
    // ====================================================================

    bool isWanted    = (state.wantedLevel > 0);
    bool isLowHealth = IsHealthLow(state);
    bool isDeadEye   = state.isDeadEyeActive;

    // Wanted edges
    if (isWanted && !m_wasWanted)
    {
        bus.Emit(EventType::WANTED_STARTED, static_cast<float>(state.wantedLevel));
        m_lastCombatTrigger = GetTickCount();
    }
    else if (!isWanted && m_wasWanted)
    {
        bus.Emit(EventType::WANTED_CLEARED);
    }

    // Low health edges
    if (isLowHealth && !m_wasLowHealth)
    {
        float healthPct = 0.0f;
        if (state.maxHealth > 100)
            healthPct = static_cast<float>(state.currentHealth - 100) / (state.maxHealth - 100);
        bus.Emit(EventType::LOW_HEALTH_ENTERED, healthPct);
    }
    else if (!isLowHealth && m_wasLowHealth)
    {
        bus.Emit(EventType::LOW_HEALTH_EXITED);
    }

    // Dead Eye edges
    if (isDeadEye && !m_wasDeadEye)
    {
        bus.Emit(EventType::DEAD_EYE_ACTIVATED);
    }
    else if (!isDeadEye && m_wasDeadEye)
    {
        bus.Emit(EventType::DEAD_EYE_DEACTIVATED);
    }

    // Store previous state for next frame
    m_wasWanted    = isWanted;
    m_wasLowHealth = isLowHealth;
    m_wasDeadEye   = isDeadEye;

    // ====================================================================
    // Determine high-level state (with priority)
    // ====================================================================

    HighLevelState newState = HighLevelState::IDLE;

    if (isDeadEye)
    {
        // Dead Eye is highest priority
        newState = HighLevelState::DEAD_EYE;
        m_lastCombatTrigger = GetTickCount();  // Dead Eye implies combat
    }
    else if (isWanted)
    {
        // Wanted implies combat
        newState = HighLevelState::COMBAT;
        m_lastCombatTrigger = GetTickCount();
    }
    else if (m_lastCombatTrigger > 0)
    {
        // Combat debounce: stay in combat for 5 seconds after last trigger
        DWORD now = GetTickCount();
        if ((now - m_lastCombatTrigger) < COMBAT_DEBOUNCE_MS)
        {
            newState = HighLevelState::COMBAT;
        }
        else
        {
            m_lastCombatTrigger = 0;  // Debounce expired
            newState = HighLevelState::IDLE;
        }
    }

    // ====================================================================
    // Emit state transition if changed
    // ====================================================================

    if (newState != m_currentState)
    {
        LOG_INFO("GameStateMachine: " + std::string(HighLevelStateName(m_currentState)) +
                 " -> " + std::string(HighLevelStateName(newState)));

        m_currentState = newState;

        // Emit the corresponding state event
        switch (newState)
        {
        case HighLevelState::IDLE:
            bus.Emit(EventType::STATE_IDLE);
            break;
        case HighLevelState::COMBAT:
            bus.Emit(EventType::STATE_COMBAT);
            break;
        case HighLevelState::DEAD_EYE:
            bus.Emit(EventType::STATE_DEAD_EYE);
            break;
        case HighLevelState::EXPLORATION:
            bus.Emit(EventType::STATE_EXPLORATION);
            break;
        default:
            break;
        }
    }
}

bool GameStateMachine::IsHealthLow(const GameState& state) const
{
    if (state.maxHealth <= 100)
        return false;

    float healthPercent = static_cast<float>(state.currentHealth - 100) /
                          static_cast<float>(state.maxHealth - 100);

    return healthPercent < LOW_HEALTH_THRESHOLD;
}
