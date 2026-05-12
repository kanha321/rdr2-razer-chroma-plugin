/*
 * Compositor.cpp — Layer blending implementation
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

        // Blend this layer's framebuffer into the output
        for (int r = 0; r < Framebuffer::ROWS; r++)
        {
            for (int c = 0; c < Framebuffer::COLS; c++)
            {
                Color base = output.GetPixel(r, c);
                Color overlay = layer->framebuffer.GetPixel(r, c);

                // Skip fully transparent pixels in alpha blend mode
                if (layer->blendMode == BlendMode::ALPHA_BLEND && overlay.a == 0)
                    continue;

                Color blended = Color::Blend(base, overlay, layer->blendMode);
                output.SetPixel(r, c, blended);
            }
        }
    }

    DWORD endTime = GetTickCount();
    m_lastCompositeTimeMs = static_cast<float>(endTime - startTime);
}
