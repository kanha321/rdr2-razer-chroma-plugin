/*
 * Compositor.cpp — Layer blending with opacity (Phase 3)
 *
 * Phase 3: Applies layer.opacity to each pixel before blending.
 * This is the single change that enables all transitions.
 */

#include "Compositor.h"
#include "../Logger.h"

void Compositor::Composite(LayerStack& layers, Framebuffer& output)
{
    DWORD startTime = GetTickCount();

    // Start with black
    output.Clear(Color::Black());

    auto sorted = layers.GetSortedLayers();

    for (Layer* layer : sorted)
    {
        if (!layer->HasContent())
            continue;

        // Phase 3: layer-level opacity scaling
        float layerOpacity = layer->opacity;
        if (layerOpacity <= 0.0f)
            continue;

        // Blend this layer's framebuffer into the output
        for (int r = 0; r < Framebuffer::ROWS; r++)
        {
            for (int c = 0; c < Framebuffer::COLS; c++)
            {
                Color base = output.GetPixel(r, c);
                Color overlay = layer->framebuffer.GetPixel(r, c);

                // Apply layer opacity to the overlay
                if (layerOpacity < 1.0f)
                {
                    overlay = overlay.WithAlpha(
                        static_cast<uint8_t>(overlay.a * layerOpacity));
                }

                // Skip fully transparent pixels
                if (overlay.a == 0)
                    continue;

                // Use ALPHA_BLEND when layer has partial opacity,
                // regardless of the layer's configured blend mode
                BlendMode effectiveMode = layer->blendMode;
                if (layerOpacity < 1.0f && effectiveMode == BlendMode::REPLACE)
                    effectiveMode = BlendMode::ALPHA_BLEND;

                Color blended = Color::Blend(base, overlay, effectiveMode);
                output.SetPixel(r, c, blended);
            }
        }
    }

    DWORD endTime = GetTickCount();
    m_lastCompositeTimeMs = static_cast<float>(endTime - startTime);
}
