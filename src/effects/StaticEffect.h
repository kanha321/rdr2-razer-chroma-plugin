/*
 * StaticEffect.h — Solid color fill effect
 *
 * The simplest effect: fills the entire framebuffer with a single color.
 * Used as a baseline/idle effect. Never finishes (persistent).
 */

#pragma once

#include "Effect.h"

class StaticEffect : public Effect
{
public:
    Color color;
    int priority;

    StaticEffect(const Color& c, int prio = 0)
        : color(c), priority(prio) {}

    void Start() override { isActive = true; }
    void Stop() override  { isActive = false; }

    void Update(float /*deltaTime*/) override
    {
        // Static — nothing to animate
    }

    void Render(Framebuffer& target) override
    {
        target.Fill(color);
    }

    const char* GetName() const override { return "StaticEffect"; }
    int GetPriority() const override { return priority; }
    bool IsFinished() const override { return false; }

    EffectDescriptor GetDescriptor() const override
    {
        return { "StaticEffect", true, false, true, BlendMode::REPLACE, 300.0f, 300.0f };
    }
};
