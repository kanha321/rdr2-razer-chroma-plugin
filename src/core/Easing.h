/*
 * Easing.h — Animation easing functions
 *
 * All functions take t in [0, 1] and return a value in [0, 1].
 * Used by effects and transitions for smooth animations.
 *
 * Reference: https://easings.net/
 */

#pragma once

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Easing
{
    // Linear — no easing
    inline float Linear(float t)
    {
        return t;
    }

    // Quadratic ease in — accelerating from zero
    inline float EaseIn(float t)
    {
        return t * t;
    }

    // Quadratic ease out — decelerating to zero
    inline float EaseOut(float t)
    {
        return t * (2.0f - t);
    }

    // Quadratic ease in-out — acceleration then deceleration
    inline float EaseInOut(float t)
    {
        if (t < 0.5f)
            return 2.0f * t * t;
        return -1.0f + (4.0f - 2.0f * t) * t;
    }

    // Sine — smooth sinusoidal curve
    inline float Sine(float t)
    {
        return 0.5f * (1.0f - cosf(static_cast<float>(M_PI) * t));
    }

    // ========================================================================
    // Easing type enum for data-driven selection
    // ========================================================================
    enum class Type
    {
        LINEAR,
        EASE_IN,
        EASE_OUT,
        EASE_IN_OUT,
        SINE
    };

    // Apply easing by type
    inline float Apply(Type type, float t)
    {
        switch (type)
        {
        case Type::LINEAR:      return Linear(t);
        case Type::EASE_IN:     return EaseIn(t);
        case Type::EASE_OUT:    return EaseOut(t);
        case Type::EASE_IN_OUT: return EaseInOut(t);
        case Type::SINE:        return Sine(t);
        default:                return Linear(t);
        }
    }
}
