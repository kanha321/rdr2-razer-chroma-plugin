/*
 * LayerStack.h — Ordered collection of render layers
 *
 * Predefined layers with fixed priorities.
 * Provides access by name and sorted iteration for compositing.
 */

#pragma once

#include "Layer.h"
#include "../Logger.h"
#include <vector>
#include <algorithm>

class LayerStack
{
public:
    LayerStack()
    {
        // Create predefined layers in priority order (lower = base)
        m_layers.emplace_back("BASE",     0,  BlendMode::REPLACE);
        m_layers.emplace_back("AMBIENT",  10, BlendMode::ALPHA_BLEND);
        m_layers.emplace_back("WEATHER",  20, BlendMode::ALPHA_BLEND);
        m_layers.emplace_back("COMBAT",   30, BlendMode::REPLACE);
        m_layers.emplace_back("FX",       35, BlendMode::ADD);        // Phase 3: transient choreography (ripples, waves)
        m_layers.emplace_back("CRITICAL", 40, BlendMode::REPLACE);
        m_layers.emplace_back("DEBUG",    50, BlendMode::REPLACE);

        LOG_INFO("LayerStack: Created " + std::to_string(m_layers.size()) + " layers");
        for (const auto& layer : m_layers)
        {
            LOG_INFO("  Layer: " + layer.name + " (priority=" + std::to_string(layer.priority) + ")");
        }
    }

    // Get layer by name (returns nullptr if not found)
    Layer* GetLayer(const std::string& name)
    {
        for (auto& layer : m_layers)
        {
            if (layer.name == name)
                return &layer;
        }
        return nullptr;
    }

    // Update all active effects
    void UpdateEffects(float deltaTime)
    {
        for (auto& layer : m_layers)
        {
            layer.Update(deltaTime);
        }
    }

    // Render all active effects into their layer framebuffers
    void RenderEffects()
    {
        for (auto& layer : m_layers)
        {
            layer.RenderEffect();
        }
    }

    // Get layers sorted by priority (ascending) for compositing
    std::vector<Layer*> GetSortedLayers()
    {
        std::vector<Layer*> sorted;
        for (auto& layer : m_layers)
            sorted.push_back(&layer);

        std::sort(sorted.begin(), sorted.end(),
            [](const Layer* a, const Layer* b) { return a->priority < b->priority; });

        return sorted;
    }

    // Count of layers with active effects
    int ActiveLayerCount() const
    {
        int count = 0;
        for (const auto& layer : m_layers)
        {
            if (layer.HasContent())
                count++;
        }
        return count;
    }

    // Clean up all effect allocations
    void Cleanup()
    {
        for (auto& layer : m_layers)
        {
            if (layer.activeEffect)
            {
                layer.activeEffect->Stop();
                delete layer.activeEffect;
                layer.activeEffect = nullptr;
            }
        }
    }

    ~LayerStack()
    {
        Cleanup();
    }

private:
    std::vector<Layer> m_layers;
};
