/*
 * ChromaRenderer.h — Framebuffer to Chroma REST API renderer
 *
 * Converts the final composited framebuffer to CHROMA_CUSTOM JSON
 * and pushes it to the Razer Chroma SDK via ChromaClient.
 *
 * Always uses CHROMA_CUSTOM (6x22 grid) — no STATIC fallback.
 * Frame diffing: only sends when pixels actually change.
 */

#pragma once

#include "../runtime/IFrameRenderer.h"
#include "../runtime/IChromaSession.h"

class ChromaRenderer : public IFrameRenderer
{
public:
    ChromaRenderer(IChromaSession& client);

    // Render the framebuffer to hardware
    // Returns true if a frame was actually sent
    bool Render(const Framebuffer& framebuffer) override;

    // Stats
    int GetFramesSent() const override { return m_framesSent; }
    int GetFramesSkipped() const override { return m_framesSkipped; }

private:
    IChromaSession& m_client;
    Framebuffer m_lastFrame;       // Previous frame for diffing
    bool m_hasLastFrame;           // Is m_lastFrame valid?
    int m_framesSent;
    int m_framesSkipped;

    // Build CHROMA_CUSTOM JSON payload from framebuffer
    std::string BuildCustomPayload(const Framebuffer& fb);
};
