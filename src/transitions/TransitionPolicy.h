/*
 * TransitionPolicy.h — Per-layer transition rules
 *
 * Defines how effects enter and exit each layer.
 * Attached to layers so the TransitionManager knows
 * what durations and easing to use.
 */

#pragma once

#include "../core/Easing.h"

struct TransitionPolicy
{
    float fadeInSeconds    = 0.3f;           // How long new effects fade in
    float fadeOutSeconds   = 0.3f;           // How long old effects fade out
    Easing::Type easing    = Easing::Type::EASE_IN_OUT;
    bool canInterrupt      = true;           // Can a new transition cut into an active one?
    bool useLayerFade      = false;          // Fade layer opacity instead of per-effect

    // Predefined policies
    static TransitionPolicy Instant()
    {
        return { 0.0f, 0.0f, Easing::Type::LINEAR, true, false };
    }

    static TransitionPolicy Smooth(float seconds = 0.3f)
    {
        return { seconds, seconds, Easing::Type::EASE_IN_OUT, true, false };
    }

    static TransitionPolicy Cinematic(float inSec = 0.5f, float outSec = 0.3f)
    {
        return { inSec, outSec, Easing::Type::SINE, true, false };
    }

    static TransitionPolicy Combat(float seconds = 0.2f)
    {
        return { seconds, seconds, Easing::Type::EASE_IN, true, false };
    }
};
