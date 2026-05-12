/*
 * Compositor.h — Blends layers into final framebuffer
 *
 * Iterates layers in priority order, applying each layer's blend mode
 * to combine them into a single output framebuffer.
 */

#pragma once

#include "LayerStack.h"
#include "../core/Framebuffer.h"

class Compositor
{
public:
    // Composite all active layers into the output framebuffer
    void Composite(LayerStack& layers, Framebuffer& output);

    // Debug: time taken for last composite (milliseconds)
    float GetLastCompositeTimeMs() const { return m_lastCompositeTimeMs; }

private:
    float m_lastCompositeTimeMs = 0.0f;
};
