/*
 * ChromaRenderer.cpp — Framebuffer to Chroma REST API implementation
 *
 * Always uses CHROMA_CUSTOM (6x22 grid). No CHROMA_STATIC fallback.
 * Chroma SDK supports max 30fps — frame diffing prevents unnecessary sends.
 */

#include "ChromaRenderer.h"
#include "../Logger.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

ChromaRenderer::ChromaRenderer(ChromaClient& client)
    : m_client(client)
    , m_hasLastFrame(false)
    , m_framesSent(0)
    , m_framesSkipped(0)
{
    LOG_INFO("ChromaRenderer: Initialized (mode=CHROMA_CUSTOM, grid=6x22)");
}

bool ChromaRenderer::Render(const Framebuffer& framebuffer)
{
    if (!m_client.IsReady())
        return false;

    // Frame diffing: skip if nothing changed
    if (m_hasLastFrame && framebuffer.Equals(m_lastFrame))
    {
        m_framesSkipped++;
        return false;
    }

    // Build CHROMA_CUSTOM payload (6x22 grid)
    std::string payload = BuildCustomPayload(framebuffer);

    // Send via ChromaClient session
    std::string response = m_client.SendSessionRequest("/keyboard", "PUT", payload);

    if (!response.empty())
    {
        int diffCount = m_hasLastFrame ? m_lastFrame.DiffCount(framebuffer) : (Framebuffer::ROWS * Framebuffer::COLS);

        m_lastFrame.CopyFrom(framebuffer);
        m_hasLastFrame = true;
        m_framesSent++;

        LOG_DEBUG("ChromaRenderer: Frame #" + std::to_string(m_framesSent) +
                  " sent (" + std::to_string(diffCount) + " pixels changed, " +
                  std::to_string(m_framesSkipped) + " skipped since last send)");
        m_framesSkipped = 0;
        return true;
    }

    LOG_ERROR("ChromaRenderer: Failed to send frame");
    return false;
}

std::string ChromaRenderer::BuildCustomPayload(const Framebuffer& fb)
{
    // Build 6x22 array of BGR ints
    json grid = json::array();

    for (int r = 0; r < Framebuffer::ROWS; r++)
    {
        json row = json::array();
        for (int c = 0; c < Framebuffer::COLS; c++)
        {
            row.push_back(fb.GetPixel(r, c).ToBGR());
        }
        grid.push_back(row);
    }

    json payload = {
        {"effect", "CHROMA_CUSTOM"},
        {"param", grid}
    };

    return payload.dump();
}
