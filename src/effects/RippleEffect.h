/*
 * RippleEffect.h — Expanding ring propagation animation (Phase 3)
 *
 * Now uses SpatialField for distance/ring calculations.
 * Supports configurable origin, distance function, and falloff.
 *
 * Parameters:
 *   - color: ripple color
 *   - speed: expansion speed (units per second)
 *   - width: ring thickness
 *   - bgColor: background color behind the ripple
 */

#pragma once

#include "Effect.h"
#include "../core/SpatialField.h"
#include <cmath>

class RippleEffect : public Effect
{
public:
    Color color;
    Color bgColor;
    float speed;
    float width;
    int priority;
    SpatialField field;

    RippleEffect(const Color& c, float spd = 15.0f, float w = 3.0f,
                 const Color& bg = Color::Black(), int prio = 30)
        : color(c), bgColor(bg), speed(spd), width(w), priority(prio)
    {
        field.maxRadius = SpatialField::MaxKeyboardDistance();
    }

    void Start() override
    {
        isActive = true;
        field.radius = 0.0f;
    }

    void Stop() override { isActive = false; }

    void Update(float deltaTime) override
    {
        field.Expand(speed, deltaTime);
    }

    void Render(Framebuffer& target) override
    {
        for (int r = 0; r < Framebuffer::ROWS; r++)
        {
            for (int c = 0; c < Framebuffer::COLS; c++)
            {
                float intensity = field.RingIntensity(r, c, width);
                if (intensity > 0.0f)
                {
                    Color pixelColor = Color::Lerp(bgColor, color, intensity);
                    target.SetPixel(r, c, pixelColor);
                }
                else
                {
                    target.SetPixel(r, c, bgColor);
                }
            }
        }
    }

    const char* GetName() const override { return "RippleEffect"; }
    int GetPriority() const override { return priority; }

    bool IsFinished() const override
    {
        return field.IsExpanded();
    }

    EffectDescriptor GetDescriptor() const override
    {
        return { "RippleEffect", true, true, true, BlendMode::ADD, 0.0f, 0.0f };
    }
};
