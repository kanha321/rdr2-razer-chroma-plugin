/*
 * Color.h — RGBA color with blending operations
 *
 * Internal color representation: RGBA (0-255 per channel)
 * Alpha channel enables layer blending.
 * Converts to BGR int for Chroma REST API output.
 *
 * Reference: RGB4R/ColorExtensions.cs — BGR formula: (B<<16)|(G<<8)|R
 * Reference: CChromaEditor/RzChromaSDKTypes.h — color storage patterns
 */

#pragma once

// MUST come before any Windows.h include to prevent min/max macro conflicts
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <cstdint>
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// Blend modes for layer compositing
// ============================================================================
enum class BlendMode
{
    REPLACE,      // Top layer completely replaces bottom
    ADD,          // Additive blending (clamped to 255)
    MULTIPLY,     // Multiplicative blending
    MAX,          // Take the brighter channel
    ALPHA_BLEND   // Standard alpha compositing
};

// ============================================================================
// Color struct
// ============================================================================
struct Color
{
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 255;  // Fully opaque by default

    // Default: black, opaque
    Color() = default;

    // RGBA constructor
    Color(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255)
        : r(r_), g(g_), b(b_), a(a_) {}

    // Construct from BGR int (Phase 1 compatibility)
    // BGR format: (B << 16) | (G << 8) | R
    static Color FromBGR(int bgr)
    {
        return Color(
            static_cast<uint8_t>(bgr & 0xFF),          // R
            static_cast<uint8_t>((bgr >> 8) & 0xFF),   // G
            static_cast<uint8_t>((bgr >> 16) & 0xFF),  // B
            255
        );
    }

    // Convert to BGR int for Chroma REST API
    int ToBGR() const
    {
        return (static_cast<int>(b) << 16) |
               (static_cast<int>(g) << 8) |
               static_cast<int>(r);
    }

    // Return copy with modified alpha
    Color WithAlpha(uint8_t newAlpha) const
    {
        return Color(r, g, b, newAlpha);
    }

    // Scale brightness (0.0 = black, 1.0 = original, >1.0 = brighter)
    Color operator*(float brightness) const
    {
        auto clamp255 = [](float v) -> uint8_t {
            if (v < 0.0f) return 0;
            if (v > 255.0f) return 255;
            return static_cast<uint8_t>(v);
        };
        return Color(
            clamp255(r * brightness),
            clamp255(g * brightness),
            clamp255(b * brightness),
            a
        );
    }

    bool operator==(const Color& other) const
    {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }

    bool operator!=(const Color& other) const
    {
        return !(*this == other);
    }

    // ========================================================================
    // Static operations
    // ========================================================================

    // Linear interpolation between two colors
    static Color Lerp(const Color& a, const Color& b, float t)
    {
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        return Color(
            static_cast<uint8_t>(a.r + (b.r - a.r) * t),
            static_cast<uint8_t>(a.g + (b.g - a.g) * t),
            static_cast<uint8_t>(a.b + (b.b - a.b) * t),
            static_cast<uint8_t>(a.a + (b.a - a.a) * t)
        );
    }

    // Blend two colors using the specified mode
    static Color Blend(const Color& base, const Color& overlay, BlendMode mode)
    {
        switch (mode)
        {
        case BlendMode::REPLACE:
            return overlay;

        case BlendMode::ADD:
        {
            int rr = (int)base.r + overlay.r; if (rr > 255) rr = 255;
            int gg = (int)base.g + overlay.g; if (gg > 255) gg = 255;
            int bb = (int)base.b + overlay.b; if (bb > 255) bb = 255;
            return Color(
                static_cast<uint8_t>(rr),
                static_cast<uint8_t>(gg),
                static_cast<uint8_t>(bb),
                255
            );
        }
        case BlendMode::MULTIPLY:
            return Color(
                static_cast<uint8_t>((base.r * overlay.r) / 255),
                static_cast<uint8_t>((base.g * overlay.g) / 255),
                static_cast<uint8_t>((base.b * overlay.b) / 255),
                255
            );

        case BlendMode::MAX:
            return Color(
                (base.r > overlay.r) ? base.r : overlay.r,
                (base.g > overlay.g) ? base.g : overlay.g,
                (base.b > overlay.b) ? base.b : overlay.b,
                255
            );

        case BlendMode::ALPHA_BLEND:
        {
            // Standard alpha compositing: out = overlay * alpha + base * (1 - alpha)
            float alpha = overlay.a / 255.0f;
            float invAlpha = 1.0f - alpha;
            return Color(
                static_cast<uint8_t>(overlay.r * alpha + base.r * invAlpha),
                static_cast<uint8_t>(overlay.g * alpha + base.g * invAlpha),
                static_cast<uint8_t>(overlay.b * alpha + base.b * invAlpha),
                255
            );
        }

        default:
            return overlay;
        }
    }

    // ========================================================================
    // Predefined colors (RGB values, NOT BGR)
    // ========================================================================
    static Color Black()   { return Color(0, 0, 0); }
    static Color White()   { return Color(255, 255, 255); }
    static Color Red()     { return Color(255, 0, 0); }
    static Color Green()   { return Color(0, 255, 0); }
    static Color Blue()    { return Color(0, 0, 255); }
    static Color Orange()  { return Color(255, 128, 0); }
    static Color Sepia()   { return Color(128, 85, 45); }   // Dead Eye color
    static Color DimWhite(){ return Color(64, 64, 64); }    // Idle color
};
