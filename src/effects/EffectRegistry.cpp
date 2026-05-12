/*
 * EffectRegistry.cpp — Declarative event-to-effect mapping implementation
 */

#include "EffectRegistry.h"
#include "../render/LayerStack.h"
#include "../Logger.h"

void EffectRegistry::Register(EventType eventType, const std::string& layerName, EffectFactory factory)
{
    m_mappings.push_back({ eventType, layerName, factory });

    std::string factoryDesc = factory ? "effect" : "CLEAR";
    LOG_INFO("EffectRegistry: Registered " + std::string(EventTypeName(eventType)) +
             " -> " + layerName + " (" + factoryDesc + ")");
}

void EffectRegistry::BindToEventBus(EventBus& bus, LayerStack& layers)
{
    // Subscribe to ALL events — the registry filters by its mapping table
    bus.SubscribeAll([this, &layers](const GameEventData& data) {
        this->OnEvent(data, layers);
    });

    LOG_INFO("EffectRegistry: Bound to EventBus with " +
             std::to_string(m_mappings.size()) + " mappings");
}

void EffectRegistry::OnEvent(const GameEventData& data, LayerStack& layers)
{
    for (const auto& mapping : m_mappings)
    {
        if (mapping.eventType != data.type)
            continue;

        Layer* layer = layers.GetLayer(mapping.layerName);
        if (!layer)
        {
            LOG_ERROR("EffectRegistry: Layer '" + mapping.layerName + "' not found!");
            continue;
        }

        // Stop current effect on this layer
        if (layer->activeEffect)
        {
            LOG_INFO("EffectRegistry: Stopping " + std::string(layer->activeEffect->GetName()) +
                     " on " + mapping.layerName);
            layer->activeEffect->Stop();
            delete layer->activeEffect;
            layer->activeEffect = nullptr;
        }

        // Activate new effect (if factory provided)
        if (mapping.factory)
        {
            Effect* newEffect = mapping.factory();
            if (newEffect)
            {
                newEffect->Start();
                layer->activeEffect = newEffect;
                LOG_INFO("EffectRegistry: Activated " + std::string(newEffect->GetName()) +
                         " on " + mapping.layerName +
                         " (event=" + std::string(EventTypeName(data.type)) + ")");
            }
        }
        else
        {
            // nullptr factory = clear the layer
            layer->framebuffer.Clear();
            LOG_INFO("EffectRegistry: Cleared " + mapping.layerName +
                     " (event=" + std::string(EventTypeName(data.type)) + ")");
        }
    }
}
