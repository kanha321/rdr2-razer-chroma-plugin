/*
 * TimelineComposer.h — Sequenced animation scheduler
 *
 * A simple fire-and-forget delay queue.
 * Schedule lambdas with delays; they fire when the countdown hits zero.
 *
 * Usage:
 *   timeline.Schedule(0.0f, [&]{ layer->FadeIn(0.3f); });
 *   timeline.Schedule(0.2f, [&]{ transitionMgr.RequestTransition(...); });
 *
 * Call Update(dt) each frame.
 */

#pragma once

#include "../Logger.h"
#include <functional>
#include <vector>
#include <string>

class TimelineComposer
{
public:
    // Schedule an action to fire after a delay (in seconds)
    void Schedule(float delaySeconds, std::function<void()> action,
                  const std::string& label = "")
    {
        m_queue.push_back({ delaySeconds, action, label, false });

        if (!label.empty())
        {
            LOG_DEBUG("TimelineComposer: Scheduled '" + label +
                      "' in " + std::to_string(delaySeconds) + "s");
        }
    }

    // Update all pending actions (call each frame)
    void Update(float deltaTime)
    {
        for (auto& entry : m_queue)
        {
            if (entry.fired) continue;

            entry.timeRemaining -= deltaTime;
            if (entry.timeRemaining <= 0.0f)
            {
                entry.fired = true;
                if (entry.action)
                {
                    entry.action();
                }

                if (!entry.label.empty())
                {
                    LOG_DEBUG("TimelineComposer: Fired '" + entry.label + "'");
                }
            }
        }

        // Remove all fired entries
        m_queue.erase(
            std::remove_if(m_queue.begin(), m_queue.end(),
                           [](const ScheduledAction& a) { return a.fired; }),
            m_queue.end()
        );
    }

    // Clear all pending actions
    void Clear()
    {
        m_queue.clear();
        LOG_DEBUG("TimelineComposer: Cleared all pending actions");
    }

    // How many actions are pending?
    size_t PendingCount() const { return m_queue.size(); }

private:
    struct ScheduledAction
    {
        float timeRemaining;
        std::function<void()> action;
        std::string label;
        bool fired;
    };

    std::vector<ScheduledAction> m_queue;
};
