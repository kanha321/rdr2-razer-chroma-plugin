/*
 * Layer.h — Single render layer with blend mode and opacity (Phase 3)
 *
 * Phase 3 additions:
 *   - Layer-level opacity (0.0–1.0) with animated fading
 *   - Outgoing effect slot for crossfade transitions
 *   - Transition-aware update/render
 */

#pragma once

#include "../core/Framebuffer.h"
#include "../core/Color.h"
#include "../effects/Effect.h"
#include "../Logger.h"
#include <string>

struct Layer
{
    std::string name;
    int priority;              // Lower = rendered first (base)
    BlendMode blendMode;
    Framebuffer framebuffer;
    Effect* activeEffect;      // Currently visible effect
    Effect* outgoingEffect;    // Old effect fading out during transition
    bool enabled;

    // ================================================================
    // Phase 3: Layer opacity
    // ================================================================
    float opacity        = 1.0f;   // Current layer opacity [0.0–1.0]
    float targetOpacity  = 1.0f;
    float opacitySpeed   = 0.0f;

    Layer(const std::string& n, int prio, BlendMode mode = BlendMode::ALPHA_BLEND)
        : name(n), priority(prio), blendMode(mode)
        , activeEffect(nullptr), outgoingEffect(nullptr), enabled(true)
    {
        framebuffer.Clear();
    }

    // Fade layer in (from 0 to 1)
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

    // Fade layer out (from current to 0)
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

    // Update layer opacity
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

    // Update effects + opacity
    void Update(float deltaTime)
    {
        UpdateOpacity(deltaTime);

        // Update outgoing effect (fading out)
        if (outgoingEffect && outgoingEffect->isActive)
        {
            outgoingEffect->UpdateOpacity(deltaTime);
            outgoingEffect->Update(deltaTime);

            // Clean up when fully faded out
            if (outgoingEffect->IsFadedOut() || outgoingEffect->IsFinished())
            {
                LOG_INFO("Layer[" + name + "]: Outgoing " +
                         std::string(outgoingEffect->GetName()) + " faded out, removing");
                outgoingEffect->Stop();
                delete outgoingEffect;
                outgoingEffect = nullptr;
            }
        }

        // Update active effect
        if (!enabled || !activeEffect || !activeEffect->isActive)
            return;

        activeEffect->UpdateOpacity(deltaTime);
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

    // Render effects into this layer's framebuffer
    void RenderEffect()
    {
        if (!enabled) return;

        // If we have both outgoing and active, blend them
        if (outgoingEffect && outgoingEffect->isActive &&
            activeEffect && activeEffect->isActive)
        {
            // Render outgoing into a temp buffer
            Framebuffer outFb;
            outgoingEffect->Render(outFb);

            // Render active into the layer framebuffer
            activeEffect->Render(framebuffer);

            // Crossfade: blend based on respective opacities
            float outOpacity = outgoingEffect->opacity;
            float inOpacity  = activeEffect->opacity;
            float total = outOpacity + inOpacity;

            if (total > 0.0f)
            {
                for (int r = 0; r < Framebuffer::ROWS; r++)
                {
                    for (int c = 0; c < Framebuffer::COLS; c++)
                    {
                        Color outColor = outFb.GetPixel(r, c);
                        Color inColor  = framebuffer.GetPixel(r, c);
                        float t = inOpacity / total;
                        framebuffer.SetPixel(r, c, Color::Lerp(outColor, inColor, t));
                    }
                }
            }
        }
        else if (activeEffect && activeEffect->isActive)
        {
            activeEffect->Render(framebuffer);
        }
        else if (outgoingEffect && outgoingEffect->isActive)
        {
            outgoingEffect->Render(framebuffer);
        }
    }

    // Does this layer have anything to composite?
    bool HasContent() const
    {
        if (!enabled || opacity <= 0.0f) return false;
        return (activeEffect != nullptr && activeEffect->isActive) ||
               (outgoingEffect != nullptr && outgoingEffect->isActive);
    }

    // Get the effective opacity (layer × dominant effect)
    float GetEffectiveOpacity() const
    {
        float effectOpacity = 1.0f;
        if (activeEffect) effectOpacity = activeEffect->opacity;
        else if (outgoingEffect) effectOpacity = outgoingEffect->opacity;
        return opacity * effectOpacity;
    }
};
