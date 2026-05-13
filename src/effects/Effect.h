/*
 * Effect.h — Base interface for all visual effects (Phase 3)
 *
 * Phase 3 additions:
 *   - opacity: 0.0–1.0 for compositing and transitions
 *   - FadeIn/FadeOut: animated opacity changes
 *   - GetDescriptor(): self-describing metadata
 *
 * Effects NEVER communicate directly with Chroma SDK.
 * They only write to framebuffers. The compositor handles opacity.
 */

#pragma once

#include "../core/Framebuffer.h"
#include "../core/Color.h"
#include "../core/AnimationTimer.h"
#include "../core/Easing.h"
#include "EffectDescriptor.h"

class Effect
{
public:
    virtual ~Effect() = default;

    // Lifecycle
    virtual void Start() = 0;
    virtual void Stop() = 0;

    // Per-frame update (deltaTime in seconds)
    virtual void Update(float deltaTime) = 0;

    // Render current state into the target framebuffer
    virtual void Render(Framebuffer& target) = 0;

    // Identity
    virtual const char* GetName() const = 0;
    virtual int GetPriority() const = 0;

    // Is this effect done? (e.g., flash finished)
    virtual bool IsFinished() const = 0;

    // Self-describing metadata (Phase 3)
    virtual EffectDescriptor GetDescriptor() const
    {
        return EffectDescriptor{ GetName(), true, false, true,
                                  BlendMode::REPLACE, 300.0f, 300.0f };
    }

    // ====================================================================
    // Opacity system (Phase 3)
    // ====================================================================

    float opacity       = 1.0f;   // Current opacity [0.0–1.0]
    float targetOpacity = 1.0f;   // Target opacity for animated fades
    float opacitySpeed  = 0.0f;   // Opacity change per second (0 = instant)

    // Fade in: set opacity to 0, animate to 1 over duration
    void FadeIn(float durationSeconds)
    {
        if (durationSeconds <= 0.0f)
        {
            opacity = 1.0f;
            targetOpacity = 1.0f;
            opacitySpeed = 0.0f;
            return;
        }
        opacity = 0.0f;
        targetOpacity = 1.0f;
        opacitySpeed = 1.0f / durationSeconds;
    }

    // Fade out: animate opacity to 0 over duration
    void FadeOut(float durationSeconds)
    {
        if (durationSeconds <= 0.0f)
        {
            opacity = 0.0f;
            targetOpacity = 0.0f;
            opacitySpeed = 0.0f;
            return;
        }
        targetOpacity = 0.0f;
        opacitySpeed = 1.0f / durationSeconds;
    }

    // Update opacity each frame (call from Update or externally)
    void UpdateOpacity(float deltaTime)
    {
        if (opacitySpeed <= 0.0f) return;

        if (opacity < targetOpacity)
        {
            opacity += opacitySpeed * deltaTime;
            if (opacity >= targetOpacity)
            {
                opacity = targetOpacity;
                opacitySpeed = 0.0f;
            }
        }
        else if (opacity > targetOpacity)
        {
            opacity -= opacitySpeed * deltaTime;
            if (opacity <= targetOpacity)
            {
                opacity = targetOpacity;
                opacitySpeed = 0.0f;
            }
        }
    }

    // Is this effect currently fading to zero?
    bool IsFadingOut() const
    {
        return targetOpacity <= 0.0f && opacitySpeed > 0.0f;
    }

    // Has the fade-out completed?
    bool IsFadedOut() const
    {
        return opacity <= 0.0f && targetOpacity <= 0.0f;
    }

    // Can this effect be interrupted by another?
    bool interruptible = true;

    // Is this effect currently active/running?
    bool isActive = false;
};
