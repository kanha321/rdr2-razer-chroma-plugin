/*
 * EffectRegistry.cpp — Routes events through TransitionManager (Phase 3)
 *
 * Instead of instant delete/create, uses TransitionManager for
 * smooth crossfade transitions between effects.
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

void EffectRegistry::BindToEventBus(IEventBus& bus, LayerStack& layers, TransitionManager& transitionMgr)
{
    bus.SubscribeAll([this, &layers, &transitionMgr](const GameEventData& data) {
        this->OnEvent(data, layers, transitionMgr);
    });

    LOG_INFO("EffectRegistry: Bound to EventBus with " +
             std::to_string(m_mappings.size()) + " mappings (transition-aware)");
}

void EffectRegistry::OnEvent(const GameEventData& data, LayerStack& layers, TransitionManager& transitionMgr)
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

        if (mapping.factory)
        {
            // Create new effect and transition to it
            Effect* newEffect = mapping.factory();
            if (newEffect)
            {
                LOG_INFO("EffectRegistry: Transitioning to " +
                         std::string(newEffect->GetName()) +
                         " on " + mapping.layerName +
                         " (event=" + std::string(EventTypeName(data.type)) + ")");

                transitionMgr.RequestTransition(layer, newEffect);
            }
        }
        else
        {
            // nullptr factory = clear the layer with fade-out
            LOG_INFO("EffectRegistry: Clearing " + mapping.layerName +
                     " (event=" + std::string(EventTypeName(data.type)) + ")");

            transitionMgr.RequestTransition(layer, nullptr);
        }
    }
}
