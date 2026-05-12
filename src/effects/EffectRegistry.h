/*
 * EffectRegistry.h — Declarative event-to-effect mapping
 *
 * Maps EventTypes to Effect instances on specific layers.
 * When an event fires, the registry creates/activates the appropriate
 * effect on the correct layer.
 *
 * Usage:
 *   registry.Register(EventType::WANTED_STARTED, "COMBAT", effectPtr);
 *   registry.Register(EventType::WANTED_CLEARED, "COMBAT", nullptr);  // clears layer
 */

#pragma once

#include "../events/EventTypes.h"
#include "../events/EventBus.h"
#include "Effect.h"
#include <string>
#include <vector>
#include <functional>

// Forward declaration
class LayerStack;

// ============================================================================
// Effect factory — creates a new effect instance
// ============================================================================
using EffectFactory = std::function<Effect*()>;

class EffectRegistry
{
public:
    // Register: when eventType fires, create effect via factory on named layer
    // Pass nullptr factory to clear the layer on that event
    void Register(EventType eventType, const std::string& layerName, EffectFactory factory);

    // Subscribe all registered events to the EventBus
    // Needs LayerStack reference to route effects to the right layers
    void BindToEventBus(EventBus& bus, LayerStack& layers);

    // Handle an incoming event — activate/deactivate effects on layers
    void OnEvent(const GameEventData& data, LayerStack& layers);

private:
    struct Mapping
    {
        EventType eventType;
        std::string layerName;
        EffectFactory factory;  // nullptr = clear the layer
    };

    std::vector<Mapping> m_mappings;
};
