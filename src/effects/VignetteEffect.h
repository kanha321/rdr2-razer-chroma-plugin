/*
 * VignetteEffect.h — Spatial vignette with breathing (Phase 3)
 *
 * Combines the breathing cycle with a spatial vignette:
 * center of keyboard is bright, edges are darker.
 * Creates a cinematic "focus" feel — perfect for Dead Eye.
 *
 * Uses SpatialField for distance-based falloff.
 */

#pragma once

#include "Effect.h"
#include "../core/SpatialField.h"
#include <cmath>

class VignetteEffect : public Effect
{
public:
    Color color;
    float cycleDuration;
    Easing::Type easing;
    float minBrightness;
    float edgeDarkness;    // How dark the edges get (0.0 = black, 1.0 = same as center)
    int priority;

    VignetteEffect(const Color& c, float cycleDur = 3.0f,
                   Easing::Type ease = Easing::Type::SINE,
                   float minBr = 0.4f, float edgeDark = 0.2f, int prio = 0)
        : color(c), cycleDuration(cycleDur), easing(ease)
        , minBrightness(minBr), edgeDarkness(edgeDark), priority(prio)
        , m_elapsed(0.0f)
    {
        m_field.originRow = Framebuffer::ROWS / 2.0f;
        m_field.originCol = Framebuffer::COLS / 2.0f;
        m_field.maxRadius = SpatialField::MaxKeyboardDistance();
        m_field.radius = m_field.maxRadius;  // Full coverage
        m_field.rowScale = 2.0f;
    }

    void Start() override
    {
        isActive = true;
        m_elapsed = 0.0f;
    }

    void Stop() override { isActive = false; }

    void Update(float deltaTime) override
    {
        m_elapsed += deltaTime;
        if (m_elapsed > cycleDuration)
            m_elapsed -= cycleDuration;
    }

    void Render(Framebuffer& target) override
    {
        // Breathing brightness cycle
        float cycleProgress = m_elapsed / cycleDuration;
        float t;
        if (cycleProgress < 0.5f)
            t = cycleProgress * 2.0f;
        else
            t = (1.0f - cycleProgress) * 2.0f;

        float eased = Easing::Apply(easing, t);
        float breathBrightness = minBrightness + (1.0f - minBrightness) * eased;

        // Render with spatial vignette — smooth falloff for a softer gradient
        for (int r = 0; r < Framebuffer::ROWS; r++)
        {
            for (int c = 0; c < Framebuffer::COLS; c++)
            {
                float distRatio = m_field.Distance(r, c) / m_field.maxRadius;
                if (distRatio > 1.0f) distRatio = 1.0f;

            // Smoothstep falloff: softer transition from center to edges
            float falloff = distRatio * distRatio * (3.0f - 2.0f * distRatio);
                float spatialFade = 1.0f - (1.0f - edgeDarkness) * falloff;
                float finalBrightness = breathBrightness * spatialFade;

                Color pixelColor = color * finalBrightness;
                target.SetPixel(r, c, pixelColor);
            }
        }
    }

    const char* GetName() const override { return "VignetteEffect"; }
    int GetPriority() const override { return priority; }
    bool IsFinished() const override { return false; }

    EffectDescriptor GetDescriptor() const override
    {
        return { "VignetteEffect", true, true, true, BlendMode::REPLACE, 500.0f, 300.0f };
    }

private:
    float m_elapsed;
    SpatialField m_field;
};
