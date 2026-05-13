/*
 * FlashEffect.h — Short burst effect
 *
 * Bright flash that fades out over a short duration.
 * Used for: gunshot hits, damage taken, etc.
 * Finishes after duration elapses.
 *
 * Parameters:
 *   - color: flash color at peak
 *   - duration: total time before complete fade (seconds)
 *   - easing: fade curve type
 */

#pragma once

#include "Effect.h"

class FlashEffect : public Effect
{
public:
    Color color;
    float duration;
    Easing::Type easing;
    int priority;

    FlashEffect(const Color& c, float dur = 0.3f, Easing::Type ease = Easing::Type::EASE_OUT, int prio = 40)
        : color(c), duration(dur), easing(ease), priority(prio)
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
        // Fade from full to zero over duration
        float progress = m_timer.Progress();
        float eased = Easing::Apply(easing, progress);
        float brightness = 1.0f - eased;  // 1.0 at start → 0.0 at end

        Color faded = color * brightness;
        target.Fill(faded);
    }

    const char* GetName() const override { return "FlashEffect"; }
    int GetPriority() const override { return priority; }
    bool IsFinished() const override { return m_timer.IsFinished(); }

    EffectDescriptor GetDescriptor() const override
    {
        return { "FlashEffect", true, false, true, BlendMode::ADD, 0.0f, 0.0f };
    }

private:
    AnimationTimer m_timer;
};
