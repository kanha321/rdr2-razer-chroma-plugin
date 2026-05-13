/*
 * SpatialField.h — Reusable spatial animation primitives
 *
 * Provides origin point, distance functions, and falloff curves
 * for any effect or transition that needs per-key spatial behavior.
 *
 * Used by: RippleEffect, RippleTransition, DissolveTransition,
 * and any future spatial effects.
 *
 * ONE implementation, many consumers.
 */

#pragma once

#include "Easing.h"
#include "Framebuffer.h"
#include <cmath>

struct SpatialField
{
    float originRow = Framebuffer::ROWS / 2.0f;
    float originCol = Framebuffer::COLS / 2.0f;
    float radius    = 0.0f;       // Current expansion radius
    float maxRadius = 30.0f;      // Max expansion (diagonal of keyboard ~24)
    float rowScale  = 2.0f;       // Scale rows to match column aspect ratio

    enum class DistanceFunc { EUCLIDEAN, MANHATTAN };
    DistanceFunc distFunc = DistanceFunc::EUCLIDEAN;

    // Calculate distance from origin to a grid cell
    float Distance(int row, int col) const
    {
        float dr = (row - originRow) * rowScale;
        float dc = static_cast<float>(col) - originCol;

        switch (distFunc)
        {
        case DistanceFunc::MANHATTAN:
            return fabsf(dr) + fabsf(dc);
        case DistanceFunc::EUCLIDEAN:
        default:
            return sqrtf(dr * dr + dc * dc);
        }
    }

    // Get intensity at (row, col) based on distance from origin and current radius
    // Returns [0, 1]: 1.0 = at origin, 0.0 = beyond radius
    float Intensity(int row, int col, Easing::Type falloff = Easing::Type::LINEAR) const
    {
        float dist = Distance(row, col);
        if (radius <= 0.0f) return 0.0f;
        if (dist >= radius) return 0.0f;

        float t = 1.0f - (dist / radius);  // 1.0 at center, 0.0 at edge
        return Easing::Apply(falloff, t);
    }

    // Get intensity for a ring (ripple) at current radius with given width
    // Returns [0, 1]: 1.0 = on the ring, 0.0 = away from ring
    float RingIntensity(int row, int col, float ringWidth) const
    {
        float dist = Distance(row, col);
        float ringDist = fabsf(dist - radius);

        if (ringDist >= ringWidth) return 0.0f;
        return 1.0f - (ringDist / ringWidth);  // Linear fade from ring center
    }

    // Is this field fully expanded beyond the keyboard?
    bool IsExpanded() const
    {
        return radius >= maxRadius;
    }

    // Update radius by speed (call each frame)
    void Expand(float speed, float deltaTime)
    {
        radius += speed * deltaTime;
    }

    // Compute max possible distance (diagonal of keyboard with row scaling)
    static float MaxKeyboardDistance(float rowScale = 2.0f)
    {
        float dr = (Framebuffer::ROWS / 2.0f) * rowScale;
        float dc = Framebuffer::COLS / 2.0f;
        return sqrtf(dr * dr + dc * dc) + 5.0f;  // +margin
    }
};
