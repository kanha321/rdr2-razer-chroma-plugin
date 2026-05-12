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

#include "../core/Framebuffer.h"
#include "../ChromaClient.h"

class ChromaRenderer
{
public:
    ChromaRenderer(ChromaClient& client);

    // Render the framebuffer to hardware
    // Returns true if a frame was actually sent
    bool Render(const Framebuffer& framebuffer);

    // Stats
    int GetFramesSent() const { return m_framesSent; }
    int GetFramesSkipped() const { return m_framesSkipped; }

private:
    ChromaClient& m_client;
    Framebuffer m_lastFrame;       // Previous frame for diffing
    bool m_hasLastFrame;           // Is m_lastFrame valid?
    int m_framesSent;
    int m_framesSkipped;

    // Build CHROMA_CUSTOM JSON payload from framebuffer
    std::string BuildCustomPayload(const Framebuffer& fb);
};
