/*
 * IEventBus.h — Event bus abstraction
 *
 * Defines a minimal interface so event producers/consumers can be
 * swapped without changing concrete implementations.
 */

#pragma once

#include "EventTypes.h"
#include <functional>

class IEventBus
{
public:
    using Callback = std::function<void(const GameEventData&)>;

    virtual ~IEventBus() = default;
    virtual void Subscribe(EventType type, Callback callback) = 0;
    virtual void SubscribeAll(Callback callback) = 0;
    virtual void Emit(EventType type, float value = 0.0f) = 0;
    virtual void ProcessQueue() = 0;
    virtual int GetLastProcessedCount() const = 0;
};
