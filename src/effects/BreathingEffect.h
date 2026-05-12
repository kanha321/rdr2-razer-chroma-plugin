/*
 * BreathingEffect.h — Smooth ambient intensity animation
 *
 * Slow, smooth brightness cycle that "breathes" in and out.
 * Used for: Dead Eye sepia overlay, ambient lighting.
 * Never finishes (loops until stopped).
 *
 * Parameters:
 *   - color: base color
 *   - cycleDuration: time for one full in-out cycle (seconds)
 *   - easing: curve type for the cycle
 *   - minBrightness: dimmest point in the cycle
 */

#pragma once

#include "Effect.h"
#include <cmath>

class BreathingEffect : public Effect
{
public:
    Color color;
    float cycleDuration;
    Easing::Type easing;
    float minBrightness;
    int priority;

    BreathingEffect(const Color& c, float cycleDur = 3.0f,
                    Easing::Type ease = Easing::Type::SINE,
                    float minBr = 0.4f, int prio = 0)
        : color(c), cycleDuration(cycleDur), easing(ease)
        , minBrightness(minBr), priority(prio)
        , m_elapsed(0.0f) {}

    void Start() override
    {
        isActive = true;
        m_elapsed = 0.0f;
    }

    void Stop() override { isActive = false; }

    void Update(float deltaTime) override
    {
        m_elapsed += deltaTime;
        // Wrap around to prevent float overflow on long sessions
        if (m_elapsed > cycleDuration)
            m_elapsed -= cycleDuration;
    }

    void Render(Framebuffer& target) override
    {
        // Progress through one cycle [0, 1]
        float cycleProgress = m_elapsed / cycleDuration;

        // Use easing for smooth in-out feel
        // Map [0, 0.5] → rising, [0.5, 1.0] → falling
        float t;
        if (cycleProgress < 0.5f)
            t = cycleProgress * 2.0f;  // 0 → 1
        else
            t = (1.0f - cycleProgress) * 2.0f;  // 1 → 0

        float eased = Easing::Apply(easing, t);
        float brightness = minBrightness + (1.0f - minBrightness) * eased;

        Color breathed = color * brightness;
        target.Fill(breathed);
    }

    const char* GetName() const override { return "BreathingEffect"; }
    int GetPriority() const override { return priority; }
    bool IsFinished() const override { return false; }  // Loops forever

private:
    float m_elapsed;
};
