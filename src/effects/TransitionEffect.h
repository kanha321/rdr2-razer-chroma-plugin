/*
 * TransitionEffect.h — Color interpolation over time
 *
 * Smoothly transitions from one color to another.
 * Finishes after duration elapses, staying at the target color.
 *
 * Parameters:
 *   - fromColor: starting color
 *   - toColor: ending color
 *   - duration: transition time (seconds)
 *   - easing: interpolation curve
 */

#pragma once

#include "Effect.h"

class TransitionEffect : public Effect
{
public:
    Color fromColor;
    Color toColor;
    float duration;
    Easing::Type easing;
    int priority;

    TransitionEffect(const Color& from, const Color& to,
                     float dur = 0.5f,
                     Easing::Type ease = Easing::Type::EASE_IN_OUT,
                     int prio = 0)
        : fromColor(from), toColor(to), duration(dur)
        , easing(ease), priority(prio)
        , m_timer(dur) {}

    void Start() override
    {
        isActive = true;
        m_timer.Reset(duration);
    }

    void Stop() override { isActive = false; }

    void Update(float deltaTime) override
    {
        m_timer.Update(deltaTime);
    }

    void Render(Framebuffer& target) override
    {
        float progress = m_timer.Progress();
        float eased = Easing::Apply(easing, progress);
        Color current = Color::Lerp(fromColor, toColor, eased);
        target.Fill(current);
    }

    const char* GetName() const override { return "TransitionEffect"; }
    int GetPriority() const override { return priority; }
    bool IsFinished() const override { return m_timer.IsFinished(); }

private:
    AnimationTimer m_timer;
};
