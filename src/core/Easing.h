/*
 * Easing.h — Animation easing functions (Phase 3 expanded)
 *
 * All functions take t in [0, 1] and return a value in [0, 1].
 * Used by effects, transitions, and timeline animations.
 *
 * Phase 3 additions: Cubic, Exponential, Bounce, Elastic
 * Reference: https://easings.net/
 */

#pragma once

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Easing
{
    // ========================================================================
    // Basic easings (Phase 2)
    // ========================================================================

    inline float Linear(float t)    { return t; }

    inline float EaseIn(float t)    { return t * t; }

    inline float EaseOut(float t)   { return t * (2.0f - t); }

    inline float EaseInOut(float t)
    {
        if (t < 0.5f) return 2.0f * t * t;
        return -1.0f + (4.0f - 2.0f * t) * t;
    }

    inline float Sine(float t)
    {
        return 0.5f * (1.0f - cosf(static_cast<float>(M_PI) * t));
    }

    // ========================================================================
    // Phase 3 additions
    // ========================================================================

    // Cubic ease in — sharper acceleration
    inline float CubicIn(float t)   { return t * t * t; }

    // Cubic ease out — sharper deceleration
    inline float CubicOut(float t)
    {
        float u = t - 1.0f;
        return u * u * u + 1.0f;
    }

    // Cubic ease in-out
    inline float CubicInOut(float t)
    {
        if (t < 0.5f) return 4.0f * t * t * t;
        float u = 2.0f * t - 2.0f;
        return 0.5f * u * u * u + 1.0f;
    }

    // Exponential ease out — fast start, long tail
    inline float ExponentialOut(float t)
    {
        if (t >= 1.0f) return 1.0f;
        return 1.0f - powf(2.0f, -10.0f * t);
    }

    // Bounce — bounces at the end (used for impact effects)
    inline float BounceOut(float t)
    {
        if (t < 1.0f / 2.75f)
            return 7.5625f * t * t;
        else if (t < 2.0f / 2.75f)
        {
            float u = t - 1.5f / 2.75f;
            return 7.5625f * u * u + 0.75f;
        }
        else if (t < 2.5f / 2.75f)
        {
            float u = t - 2.25f / 2.75f;
            return 7.5625f * u * u + 0.9375f;
        }
        else
        {
            float u = t - 2.625f / 2.75f;
            return 7.5625f * u * u + 0.984375f;
        }
    }

    // Elastic — overshoot + oscillation settle
    inline float ElasticOut(float t)
    {
        if (t <= 0.0f) return 0.0f;
        if (t >= 1.0f) return 1.0f;
        return powf(2.0f, -10.0f * t) * sinf((t - 0.075f) * (2.0f * static_cast<float>(M_PI)) / 0.3f) + 1.0f;
    }

    // ========================================================================
    // Type enum (data-driven selection)
    // ========================================================================
    enum class Type
    {
        LINEAR,
        EASE_IN,
        EASE_OUT,
        EASE_IN_OUT,
        SINE,
        // Phase 3
        CUBIC_IN,
        CUBIC_OUT,
        CUBIC_IN_OUT,
        EXPONENTIAL_OUT,
        BOUNCE_OUT,
        ELASTIC_OUT
    };

    inline float Apply(Type type, float t)
    {
        switch (type)
        {
        case Type::LINEAR:          return Linear(t);
        case Type::EASE_IN:         return EaseIn(t);
        case Type::EASE_OUT:        return EaseOut(t);
        case Type::EASE_IN_OUT:     return EaseInOut(t);
        case Type::SINE:            return Sine(t);
        case Type::CUBIC_IN:        return CubicIn(t);
        case Type::CUBIC_OUT:       return CubicOut(t);
        case Type::CUBIC_IN_OUT:    return CubicInOut(t);
        case Type::EXPONENTIAL_OUT: return ExponentialOut(t);
        case Type::BOUNCE_OUT:      return BounceOut(t);
        case Type::ELASTIC_OUT:     return ElasticOut(t);
        default:                    return Linear(t);
        }
    }

    inline const char* TypeName(Type type)
    {
        switch (type)
        {
        case Type::LINEAR:          return "LINEAR";
        case Type::EASE_IN:         return "EASE_IN";
        case Type::EASE_OUT:        return "EASE_OUT";
        case Type::EASE_IN_OUT:     return "EASE_IN_OUT";
        case Type::SINE:            return "SINE";
        case Type::CUBIC_IN:        return "CUBIC_IN";
        case Type::CUBIC_OUT:       return "CUBIC_OUT";
        case Type::CUBIC_IN_OUT:    return "CUBIC_IN_OUT";
        case Type::EXPONENTIAL_OUT: return "EXPONENTIAL_OUT";
        case Type::BOUNCE_OUT:      return "BOUNCE_OUT";
        case Type::ELASTIC_OUT:     return "ELASTIC_OUT";
        default:                    return "UNKNOWN";
        }
    }
}
