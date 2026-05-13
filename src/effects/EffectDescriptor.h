/*
 * EffectDescriptor.h — Self-describing effect metadata
 *
 * Every effect exposes a descriptor so the transition system,
 * compositor, and future AI tools know what capabilities it has.
 *
 * This makes the system self-describing and enables data-driven
 * transition selection.
 */

#pragma once

#include "../core/Color.h"

struct EffectDescriptor
{
    const char* name            = "Unknown";
    bool supportsOpacity        = true;    // Can this effect be faded?
    bool supportsSpatial        = false;   // Does it use per-key spatial data?
    bool transitionable         = true;    // Can transitions be applied?
    BlendMode preferredBlendMode = BlendMode::REPLACE;
    float defaultFadeInMs       = 300.0f;  // Suggested fade-in duration
    float defaultFadeOutMs      = 300.0f;  // Suggested fade-out duration
};
