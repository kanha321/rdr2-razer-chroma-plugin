/*
 * EventBus.cpp — Publish/subscribe event system implementation
 */

#include "EventBus.h"
#include "../Logger.h"

void EventBus::Subscribe(EventType type, Callback callback)
{
    m_subscribers.push_back({ type, callback });
    LOG_INFO("EventBus: Subscribed to " + std::string(EventTypeName(type)));
}

void EventBus::SubscribeAll(Callback callback)
{
    m_globalSubscribers.push_back(callback);
    LOG_INFO("EventBus: Global subscriber added (receives all events)");
}

void EventBus::Emit(EventType type, float value)
{
    GameEventData data;
    data.type = type;
    data.timestamp = GetTickCount();
    data.value = value;

    m_queue.push_back(data);
    LOG_INFO("EventBus: Emit " + std::string(EventTypeName(type)) +
             " (t=" + std::to_string(data.timestamp) +
             ", val=" + std::to_string(value) + ")");
}

void EventBus::ProcessQueue()
{
    m_lastProcessedCount = static_cast<int>(m_queue.size());

    for (const auto& event : m_queue)
    {
        // Deliver to type-specific subscribers
        for (const auto& sub : m_subscribers)
        {
            if (sub.type == event.type)
            {
                sub.callback(event);
            }
        }

        // Deliver to global subscribers (EffectRegistry)
        for (const auto& globalCb : m_globalSubscribers)
        {
            globalCb(event);
        }
    }

    m_queue.clear();
}
