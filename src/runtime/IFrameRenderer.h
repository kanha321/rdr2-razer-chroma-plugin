/*
 * IFrameRenderer.h — Framebuffer renderer abstraction
 */

#pragma once

#include "../core/Framebuffer.h"

class IFrameRenderer
{
public:
    virtual ~IFrameRenderer() = default;
    virtual bool Render(const Framebuffer& framebuffer) = 0;
    virtual int GetFramesSent() const = 0;
    virtual int GetFramesSkipped() const = 0;
};
