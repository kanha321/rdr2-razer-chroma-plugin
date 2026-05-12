/*
 * PulseEffect.h — Brightness oscillation effect
 *
 * Oscillates color brightness using a sine wave.
 * Used for: wanted level (pulsing red), low health (pulsing orange).
 * Never finishes (loops until stopped).
 *
 * Parameters:
 *   - color: base color at full brightness
 *   - frequency: oscillation speed in Hz (cycles per second)
 *   - minBrightness: minimum brightness multiplier (0.0 = full off, 1.0 = no pulse)
 */

#pragma once

#include "Effect.h"
#include <cmath>

class PulseEffect : public Effect
{
public:
    Color color;
    float frequency;      // Hz
    float minBrightness;  // 0.0 to 1.0
    int priority;

    PulseEffect(const Color& c, float freq = 2.0f, float minBr = 0.3f, int prio = 0)
        : color(c), frequency(freq), minBrightness(minBr), priority(prio)
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
    }

    void Render(Framebuffer& target) override
    {
        // Sine wave oscillation: maps [0, 2π] → [-1, 1] → [minBrightness, 1.0]
        float wave = sinf(m_elapsed * frequency * 2.0f * static_cast<float>(M_PI));
        float brightness = minBrightness + (1.0f - minBrightness) * (0.5f + 0.5f * wave);

        Color pulsed = color * brightness;
        target.Fill(pulsed);
    }

    const char* GetName() const override { return "PulseEffect"; }
    int GetPriority() const override { return priority; }
    bool IsFinished() const override { return false; }  // Loops forever

private:
    float m_elapsed;
};
