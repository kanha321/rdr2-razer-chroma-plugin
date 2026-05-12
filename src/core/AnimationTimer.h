/*
 * AnimationTimer.h — Lightweight delta-time animation helper
 *
 * Used by effects to track elapsed time, compute progress,
 * and determine when an animation has finished.
 * No heap allocations — designed to be embedded in effect structs.
 */

#pragma once

#include <algorithm>

struct AnimationTimer
{
    float elapsed  = 0.0f;    // Time elapsed since start (seconds)
    float duration = 1.0f;    // Total duration (seconds), 0 = infinite

    AnimationTimer() = default;

    explicit AnimationTimer(float dur)
        : elapsed(0.0f), duration(dur) {}

    // Update timer by delta time (seconds)
    void Update(float deltaTime)
    {
        elapsed += deltaTime;
    }

    // Progress in [0, 1], clamped
    // Returns 1.0 if duration is 0 (instant)
    float Progress() const
    {
        if (duration <= 0.0f)
            return 1.0f;
        float p = elapsed / duration;
        return (p < 1.0f) ? p : 1.0f;
    }

    // Has the timer exceeded its duration?
    // Always false if duration is 0 (infinite)
    bool IsFinished() const
    {
        if (duration <= 0.0f)
            return false;  // Infinite duration never finishes
        return elapsed >= duration;
    }

    // Reset to beginning
    void Reset()
    {
        elapsed = 0.0f;
    }

    // Reset with new duration
    void Reset(float newDuration)
    {
        elapsed = 0.0f;
        duration = newDuration;
    }
};
