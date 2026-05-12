/*
 * Effect.h — Base interface for all visual effects
 *
 * Effects are reusable visual behaviors that:
 *   - Receive Update(deltaTime) each frame
 *   - Render their output to a Framebuffer
 *   - Report their priority and lifecycle state
 *
 * Effects NEVER communicate directly with Chroma SDK.
 * They only write to framebuffers. This separation is mandatory.
 *
 * Reference: CChromaEditor/AnimationBase.h — Update/Play/Stop pattern
 */

#pragma once

#include "../core/Framebuffer.h"
#include "../core/Color.h"
#include "../core/AnimationTimer.h"
#include "../core/Easing.h"

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

    // Can this effect be interrupted by another?
    bool interruptible = true;

    // Is this effect currently active/running?
    bool isActive = false;
};
