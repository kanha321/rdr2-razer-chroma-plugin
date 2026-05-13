/*
 * EffectRegistry.h — Declarative event-to-effect mapping (Phase 3)
 *
 * Routes events through TransitionManager for smooth crossfades
 * instead of instant effect swaps.
 */

#pragma once

#include "../events/EventTypes.h"
#include "../events/IEventBus.h"
#include "../transitions/TransitionManager.h"
#include "Effect.h"
#include <string>
#include <vector>
#include <functional>

// Forward declaration
class LayerStack;

using EffectFactory = std::function<Effect*()>;

class EffectRegistry
{
public:
    // Register: when eventType fires, create effect via factory on named layer
    void Register(EventType eventType, const std::string& layerName, EffectFactory factory);

    // Subscribe all registered events to the EventBus
    void BindToEventBus(IEventBus& bus, LayerStack& layers, TransitionManager& transitionMgr);

    // Handle an incoming event
    void OnEvent(const GameEventData& data, LayerStack& layers, TransitionManager& transitionMgr);

private:
    struct Mapping
    {
        EventType eventType;
        std::string layerName;
        EffectFactory factory;
    };

    std::vector<Mapping> m_mappings;
};
