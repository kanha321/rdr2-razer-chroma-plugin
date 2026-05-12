/*
 * RippleEffect.h — Expanding ring propagation animation
 *
 * Expands a colored ring from the center of the keyboard outward.
 * The ring has configurable width and speed.
 * Finishes when the ring has passed beyond all keys.
 *
 * Parameters:
 *   - color: ripple color
 *   - speed: expansion speed (columns per second)
 *   - width: ring thickness (in columns)
 *   - bgColor: background color behind the ripple
 */

#pragma once

#include "Effect.h"
#include <cmath>

class RippleEffect : public Effect
{
public:
    Color color;
    Color bgColor;
    float speed;
    float width;
    int priority;

    RippleEffect(const Color& c, float spd = 15.0f, float w = 3.0f,
                 const Color& bg = Color::Black(), int prio = 30)
        : color(c), bgColor(bg), speed(spd), width(w), priority(prio)
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
        // Center of keyboard
        float centerRow = Framebuffer::ROWS / 2.0f;
        float centerCol = Framebuffer::COLS / 2.0f;

        // Current ripple radius (expands outward)
        float radius = m_elapsed * speed;

        for (int r = 0; r < Framebuffer::ROWS; r++)
        {
            for (int c = 0; c < Framebuffer::COLS; c++)
            {
                // Distance from center (normalize rows to match column scale)
                float dr = (r - centerRow) * 2.0f;  // Scale rows (keyboard is wider than tall)
                float dc = static_cast<float>(c) - centerCol;
                float dist = sqrtf(dr * dr + dc * dc);

                // Is this pixel within the ring?
                float ringDist = fabsf(dist - radius);

                if (ringDist < width)
                {
                    // Fade based on distance from ring center
                    float fade = 1.0f - (ringDist / width);
                    Color pixelColor = Color::Lerp(bgColor, color, fade);
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
        // Done when ring has passed beyond all keys
        float maxDist = sqrtf(static_cast<float>(
            Framebuffer::ROWS * Framebuffer::ROWS * 4 +
            Framebuffer::COLS * Framebuffer::COLS));
        float radius = m_elapsed * speed;
        return radius > (maxDist + width);
    }

private:
    float m_elapsed;
};
