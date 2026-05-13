/*
 * TransitionManager.h — Orchestrates effect transitions on layers
 *
 * Replaces the instant swap in EffectRegistry with smooth crossfades.
 * Reads the TransitionPolicy from each layer to determine durations.
 *
 * This is intentionally thin — the actual blending math lives in
 * Layer::RenderEffect() which crossfades the outgoing/active slots.
 */

#pragma once

#include "../render/Layer.h"
#include "../render/LayerStack.h"
#include "../effects/Effect.h"
#include "TransitionPolicy.h"
#include "../Logger.h"
#include <unordered_map>
#include <string>

class TransitionManager
{
public:
    // Set the transition policy for a specific layer
    void SetPolicy(const std::string& layerName, const TransitionPolicy& policy)
    {
        m_policies[layerName] = policy;
        LOG_INFO("TransitionManager: Set policy for " + layerName +
                 " (fadeIn=" + std::to_string(policy.fadeInSeconds) +
                 "s, fadeOut=" + std::to_string(policy.fadeOutSeconds) + "s)");
    }

    // Get policy for a layer (returns default if not set)
    TransitionPolicy GetPolicy(const std::string& layerName) const
    {
        auto it = m_policies.find(layerName);
        if (it != m_policies.end())
            return it->second;
        return TransitionPolicy::Smooth();  // Default
    }

    // Request a new effect on a layer, with transition
    void RequestTransition(Layer* layer, Effect* newEffect)
    {
        if (!layer) return;

        TransitionPolicy policy = GetPolicy(layer->name);

        // ── Handle interruption ──
        // If there's already a transition in progress (outgoing exists)
        if (layer->outgoingEffect)
        {
            if (!policy.canInterrupt)
            {
                LOG_INFO("TransitionManager: Blocked transition on " + layer->name +
                         " (non-interruptible)");
                if (newEffect)
                {
                    delete newEffect;
                }
                return;
            }

            // Kill the current outgoing effect immediately
            LOG_INFO("TransitionManager: Interrupting transition on " + layer->name);
            layer->outgoingEffect->Stop();
            delete layer->outgoingEffect;
            layer->outgoingEffect = nullptr;
        }

        // ── Case 1: Clear the layer (newEffect is nullptr) ──
        if (!newEffect)
        {
            if (layer->activeEffect)
            {
                if (policy.fadeOutSeconds > 0.0f)
                {
                    // Move to outgoing slot, it will fade and self-destruct
                    layer->outgoingEffect = layer->activeEffect;
                    layer->outgoingEffect->FadeOut(policy.fadeOutSeconds);
                    layer->activeEffect = nullptr;

                    LOG_INFO("TransitionManager: Fading out " +
                             std::string(layer->outgoingEffect->GetName()) +
                             " on " + layer->name + " (" +
                             std::to_string(policy.fadeOutSeconds) + "s)");
                }
                else
                {
                    // Instant clear
                    layer->activeEffect->Stop();
                    delete layer->activeEffect;
                    layer->activeEffect = nullptr;
                    layer->framebuffer.Clear();
                    LOG_INFO("TransitionManager: Instant clear on " + layer->name);
                }
            }
            return;
        }

        // ── Case 2: Transition to new effect ──
        if (layer->activeEffect)
        {
            if (policy.fadeOutSeconds > 0.0f)
            {
                // Move old to outgoing, fade it out
                layer->outgoingEffect = layer->activeEffect;
                layer->outgoingEffect->FadeOut(policy.fadeOutSeconds);

                LOG_INFO("TransitionManager: Crossfade on " + layer->name +
                         " (" + std::string(layer->outgoingEffect->GetName()) +
                         " → " + std::string(newEffect->GetName()) + ", " +
                         std::to_string(policy.fadeInSeconds) + "s)");
            }
            else
            {
                // Instant removal of old
                layer->activeEffect->Stop();
                delete layer->activeEffect;
                LOG_INFO("TransitionManager: Instant swap on " + layer->name +
                         " → " + std::string(newEffect->GetName()));
            }
        }
        else
        {
            LOG_INFO("TransitionManager: Activating " +
                     std::string(newEffect->GetName()) +
                     " on " + layer->name + " (" +
                     std::to_string(policy.fadeInSeconds) + "s fade)");
        }

        // Start the new effect
        newEffect->Start();

        if (policy.fadeInSeconds > 0.0f)
        {
            newEffect->FadeIn(policy.fadeInSeconds);
        }

        // If using layer-level fade, animate the layer opacity too
        if (policy.useLayerFade && layer->opacity < 1.0f)
        {
            layer->FadeIn(policy.fadeInSeconds);
        }

        layer->activeEffect = newEffect;
    }

    // Request fading a layer out entirely (then clearing it)
    void FadeOutLayer(Layer* layer, float durationSeconds)
    {
        if (!layer) return;
        layer->FadeOut(durationSeconds);
        LOG_INFO("TransitionManager: Layer " + layer->name +
                 " fading out (" + std::to_string(durationSeconds) + "s)");
    }

    // Request fading a layer in
    void FadeInLayer(Layer* layer, float durationSeconds)
    {
        if (!layer) return;
        layer->FadeIn(durationSeconds);
        LOG_INFO("TransitionManager: Layer " + layer->name +
                 " fading in (" + std::to_string(durationSeconds) + "s)");
    }

    // Per-frame update — nothing needed here; Layer::Update() handles fading
    void Update(float /*deltaTime*/)
    {
        // All fade logic is driven by Layer::Update() calling
        // Effect::UpdateOpacity() and Layer::UpdateOpacity().
        // TransitionManager is stateless after RequestTransition().
    }

private:
    std::unordered_map<std::string, TransitionPolicy> m_policies;
};
