/*
 * Layer.h — Single render layer with blend mode
 *
 * Each layer has its own framebuffer and an active effect.
 * The compositor blends layers in priority order.
 */

#pragma once

#include "../core/Framebuffer.h"
#include "../core/Color.h"
#include "../effects/Effect.h"
#include <string>

struct Layer
{
    std::string name;
    int priority;           // Lower = rendered first (base)
    BlendMode blendMode;
    Framebuffer framebuffer;
    Effect* activeEffect;   // Currently rendering effect (owned by EffectRegistry)
    bool enabled;

    Layer(const std::string& n, int prio, BlendMode mode = BlendMode::ALPHA_BLEND)
        : name(n), priority(prio), blendMode(mode)
        , activeEffect(nullptr), enabled(true)
    {
        framebuffer.Clear();
    }

    // Update the active effect
    void Update(float deltaTime)
    {
        if (!enabled || !activeEffect || !activeEffect->isActive)
            return;

        activeEffect->Update(deltaTime);

        // Auto-cleanup finished effects
        if (activeEffect->IsFinished())
        {
            activeEffect->Stop();
            delete activeEffect;
            activeEffect = nullptr;
            framebuffer.Clear();
        }
    }

    // Render the active effect into this layer's framebuffer
    void RenderEffect()
    {
        if (!enabled || !activeEffect || !activeEffect->isActive)
            return;

        activeEffect->Render(framebuffer);
    }

    // Does this layer have anything to composite?
    bool HasContent() const
    {
        return enabled && activeEffect != nullptr && activeEffect->isActive;
    }
};
