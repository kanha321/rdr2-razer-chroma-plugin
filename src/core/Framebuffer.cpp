/*
 * Framebuffer.cpp — Virtual 6×22 per-key render target implementation
 */

#include "Framebuffer.h"

Framebuffer::Framebuffer()
{
    Clear();
}

void Framebuffer::Clear(const Color& color)
{
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            m_pixels[r][c] = color;
}

void Framebuffer::SetPixel(int row, int col, const Color& color)
{
    if (row >= 0 && row < ROWS && col >= 0 && col < COLS)
        m_pixels[row][col] = color;
}

Color Framebuffer::GetPixel(int row, int col) const
{
    if (row >= 0 && row < ROWS && col >= 0 && col < COLS)
        return m_pixels[row][col];
    return Color::Black();
}

void Framebuffer::Fill(const Color& color)
{
    Clear(color);
}

void Framebuffer::CopyFrom(const Framebuffer& other)
{
    memcpy(m_pixels, other.m_pixels, sizeof(m_pixels));
}

bool Framebuffer::Equals(const Framebuffer& other) const
{
    return memcmp(m_pixels, other.m_pixels, sizeof(m_pixels)) == 0;
}

int Framebuffer::DiffCount(const Framebuffer& other) const
{
    int count = 0;
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            if (m_pixels[r][c] != other.m_pixels[r][c])
                count++;
    return count;
}
