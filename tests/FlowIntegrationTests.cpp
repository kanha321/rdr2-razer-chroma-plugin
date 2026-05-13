/*
 * FlowIntegrationTests.cpp — minimal integration tests for pipeline wiring
 *
 * No external test framework; exits non-zero on failure.
 */

#include <cstdio>
#include <string>

#include "events/EventBus.h"
#include "events/GameStateMachine.h"
#include "effects/EffectRegistry.h"
#include "effects/PulseEffect.h"
#include "render/LayerStack.h"
#include "render/Compositor.h"
#include "render/ChromaRenderer.h"
#include "transitions/TransitionManager.h"
#include "transitions/TransitionPolicy.h"
#include "runtime/IChromaSession.h"
#include "core/Framebuffer.h"
#include "core/Color.h"

struct TestContext
{
    int failed = 0;

    void Check(bool condition, const char* message)
    {
        if (condition)
        {
            std::printf("PASS: %s\n", message);
        }
        else
        {
            std::printf("FAIL: %s\n", message);
            failed++;
        }
    }
};

class FakeChromaSession : public IChromaSession
{
public:
    int sendCount = 0;
    bool ready = true;
    size_t lastBodySize = 0;
    std::string lastPath;

    bool Initialize() override { ready = true; return true; }
    void Heartbeat() override {}
    bool SetKeyboardColor(int /*bgrColor*/) override { return true; }
    void Shutdown() override { ready = false; }
    bool IsReady() const override { return ready; }

    std::string SendSessionRequest(
        const std::string& subpath,
        const std::string& method,
        const std::string& body = ""
    ) override
    {
        (void)method;
        sendCount++;
        lastPath = subpath;
        lastBodySize = body.size();
        return "ok";
    }
};

static GameState MakeState(bool valid, int wanted, int currentHealth, int maxHealth, bool deadEye)
{
    GameState state;
    state.isPlayerValid = valid;
    state.wantedLevel = wanted;
    state.currentHealth = currentHealth;
    state.maxHealth = maxHealth;
    state.isDeadEyeActive = deadEye;
    return state;
}

static void RunEventFlowTest(TestContext& ctx)
{
    EventBus bus;
    GameStateMachine sm;
    LayerStack layers;
    EffectRegistry registry;
    TransitionManager transitions;

    transitions.SetPolicy("COMBAT", TransitionPolicy::Instant());

    registry.Register(EventType::WANTED_STARTED, "COMBAT", []() -> Effect* {
        return new PulseEffect(Color::Red(), 2.0f, 0.3f, 0);
    });
    registry.Register(EventType::WANTED_CLEARED, "COMBAT", nullptr);
    registry.BindToEventBus(bus, layers, transitions);

    GameState s0 = MakeState(true, 0, 200, 200, false);
    sm.Update(s0, bus);
    bus.ProcessQueue();

    GameState s1 = MakeState(true, 1, 200, 200, false);
    sm.Update(s1, bus);
    bus.ProcessQueue();

    layers.UpdateEffects(0.016f);
    layers.RenderEffects();

    Layer* combat = layers.GetLayer("COMBAT");
    ctx.Check(combat != nullptr, "COMBAT layer exists");
    ctx.Check(combat && combat->activeEffect != nullptr, "WANTED activates COMBAT effect");
    if (combat && combat->activeEffect)
    {
        ctx.Check(std::string(combat->activeEffect->GetName()) == "PulseEffect",
                  "COMBAT effect is PulseEffect");
    }

    GameState s2 = MakeState(true, 0, 200, 200, false);
    sm.Update(s2, bus);
    bus.ProcessQueue();

    layers.UpdateEffects(0.016f);
    layers.RenderEffects();

    ctx.Check(combat && combat->activeEffect == nullptr, "WANTED cleared removes COMBAT effect");
}

static void RunRendererDiffTest(TestContext& ctx)
{
    FakeChromaSession session;
    ChromaRenderer renderer(session);

    Framebuffer fb;
    fb.Clear(Color::Black());

    bool first = renderer.Render(fb);
    bool second = renderer.Render(fb);

    ctx.Check(first, "First frame sent");
    ctx.Check(!second, "Second frame skipped");
    ctx.Check(session.sendCount == 1, "Session called once for identical frames");
    ctx.Check(renderer.GetFramesSent() == 1, "FramesSent == 1");
    ctx.Check(renderer.GetFramesSkipped() == 1, "FramesSkipped == 1");
    ctx.Check(session.lastBodySize > 0, "Payload body is non-empty");
}

int main()
{
    TestContext ctx;

    RunEventFlowTest(ctx);
    RunRendererDiffTest(ctx);

    if (ctx.failed == 0)
    {
        std::printf("All tests passed.\n");
        return 0;
    }

    std::printf("Tests failed: %d\n", ctx.failed);
    return 1;
}
