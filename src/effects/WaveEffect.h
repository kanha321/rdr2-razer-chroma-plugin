/*
 * WaveEffect.h — Horizontal color wave sweep (Phase 3)
 *
 * A color wave that sweeps across the keyboard left-to-right
 * (or right-to-left) with configurable speed and trail.
 * Used for ambient/idle variation, combat enter flourish.
 *
 * Loops continuously. Demonstrates per-key spatial animation.
 */

#pragma once

#include "Effect.h"
#include <cmath>

class WaveEffect : public Effect
{
public:
    Color color;
    Color bgColor;
    float speed;        // Columns per second
    float trailWidth;   // How many columns the wave spans
    int priority;
    bool rightToLeft;
    bool loop;

    WaveEffect(const Color& c, float spd = 8.0f, float trail = 6.0f,
               const Color& bg = Color::Black(),
               bool rtl = false, int prio = 0, bool loop_ = true)
        : color(c), bgColor(bg), speed(spd), trailWidth(trail)
        , priority(prio), rightToLeft(rtl), loop(loop_), m_position(0.0f) {}

    void Start() override
    {
        isActive = true;
        m_position = rightToLeft ? static_cast<float>(Framebuffer::COLS) : -trailWidth;
    }

    void Stop() override { isActive = false; }

    void Update(float deltaTime) override
    {
        if (rightToLeft)
            m_position -= speed * deltaTime;
        else
            m_position += speed * deltaTime;

        if (loop)
        {
            // Wrap around
            float total = static_cast<float>(Framebuffer::COLS) + trailWidth * 2.0f;
            (void)total;
            if (!rightToLeft && m_position > Framebuffer::COLS + trailWidth)
                m_position = -trailWidth;
            else if (rightToLeft && m_position < -trailWidth)
                m_position = static_cast<float>(Framebuffer::COLS) + trailWidth;
        }
    }

    void Render(Framebuffer& target) override
    {
        for (int r = 0; r < Framebuffer::ROWS; r++)
        {
            for (int c = 0; c < Framebuffer::COLS; c++)
            {
                float dist = fabsf(static_cast<float>(c) - m_position);
                if (dist < trailWidth)
                {
                    float intensity = 1.0f - (dist / trailWidth);
                    // Smooth the wave with sine
                    intensity = intensity * intensity;
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

    const char* GetName() const override { return "WaveEffect"; }
    int GetPriority() const override { return priority; }
    bool IsFinished() const override
    {
        if (loop)
            return false;
        if (rightToLeft)
            return m_position < -trailWidth;
        return m_position > Framebuffer::COLS + trailWidth;
    }

    EffectDescriptor GetDescriptor() const override
    {
        return { "WaveEffect", true, true, true, BlendMode::REPLACE, 300.0f, 300.0f };
    }

private:
    float m_position;
};
