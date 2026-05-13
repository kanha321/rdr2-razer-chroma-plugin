/*
 * IStateMachine.h — High-level state machine abstraction
 */

#pragma once

#include "HighLevelState.h"
#include "IEventBus.h"
#include "../GameState.h"

class IStateMachine
{
public:
    virtual ~IStateMachine() = default;
    virtual void Update(const GameState& state, IEventBus& bus) = 0;
    virtual HighLevelState GetCurrentState() const = 0;
};
