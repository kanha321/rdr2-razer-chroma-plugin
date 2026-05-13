/*
 * EventBus.h — Publish/subscribe event system
 *
 * Decouples game detection from rendering/effects.
 * Events are queued during a frame and processed in batch.
 * Subscribers receive events in emission order.
 */

#pragma once

#include "IEventBus.h"
#include <vector>

class EventBus : public IEventBus
{
public:
    // Callback type: receives event data
    using Callback = IEventBus::Callback;

    // Subscribe to a specific event type
    void Subscribe(EventType type, Callback callback) override;

    // Subscribe to ALL events (used by EffectRegistry)
    void SubscribeAll(Callback callback) override;

    // Emit an event (queued for batch processing)
    void Emit(EventType type, float value = 0.0f) override;

    // Process all queued events — call once per frame
    void ProcessQueue() override;

    // Get count of events processed this frame (for debug logging)
    int GetLastProcessedCount() const override { return m_lastProcessedCount; }

private:
    struct Subscriber
    {
        EventType type;
        Callback callback;
    };

    std::vector<Subscriber> m_subscribers;
    std::vector<Callback> m_globalSubscribers;  // Receive ALL events
    std::vector<GameEventData> m_queue;
    int m_lastProcessedCount = 0;
};
