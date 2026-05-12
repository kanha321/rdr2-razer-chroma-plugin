/*
 * Framebuffer.h — Virtual 6×22 per-key render target
 *
 * Represents the keyboard as a grid of colors.
 * Effects render into framebuffers, the compositor blends them,
 * and the renderer converts the final buffer to Chroma API format.
 *
 * Dimensions: 6 rows × 22 columns (Razer Chroma standard)
 * Reference: Chroma REST API CHROMA_CUSTOM specification
 */

#pragma once

#include "Color.h"
#include <cstring>

class Framebuffer
{
public:
    static constexpr int ROWS = 6;
    static constexpr int COLS = 22;

    Framebuffer();

    // Fill entire buffer with a single color
    void Clear(const Color& color = Color::Black());

    // Set/get individual pixel
    void SetPixel(int row, int col, const Color& color);
    Color GetPixel(int row, int col) const;

    // Fill entire buffer with one color (alias for Clear)
    void Fill(const Color& color);

    // Copy contents from another framebuffer
    void CopyFrom(const Framebuffer& other);

    // Compare two framebuffers for equality (used for frame diffing)
    bool Equals(const Framebuffer& other) const;

    // Count how many pixels differ from another buffer
    int DiffCount(const Framebuffer& other) const;

    // Direct access to pixel data (for compositor and renderer)
    Color* operator[](int row) { return m_pixels[row]; }
    const Color* operator[](int row) const { return m_pixels[row]; }

private:
    Color m_pixels[ROWS][COLS];
};
