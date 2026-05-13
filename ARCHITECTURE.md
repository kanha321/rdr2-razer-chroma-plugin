# RDR2ChromaSync — Architecture Guide

An ASI plugin that drives Razer Chroma keyboard lighting in real-time based on Red Dead Redemption 2 gameplay events.

## How It Works (30-Second Version)

```
Game State ──→ Event Bus ──→ State Machine ──→ Effects ──→ Layers ──→ Compositor ──→ Framebuffer ──→ Chroma SDK
  (natives)     (pub/sub)     (debounced)      (animated)   (blend)    (merge)        (6×22 grid)     (REST API)
```

The plugin runs on ScriptHookRDR2's fiber thread. Every frame (~30 FPS):

1. **Poll** the game for player state (health, wanted level, Dead Eye)
2. **Detect** state transitions and emit events
3. **Activate** visual effects on the appropriate render layer
4. **Animate** effects using real-time delta
5. **Composite** all layers into a single 6×22 pixel grid
6. **Push** the grid to Razer Chroma via localhost REST API

---

## Project Structure

```
src/
├── dllmain.cpp              # Entry point — DllMain + ScriptMain render loop
├── Config.h                 # Compile-time constants and backward-compatible aliases
├── JsonConfig.h             # Runtime JSON config loader (reads RDR2ChromaSync_config.json)
├── Logger.h                 # File logger (writes to RDR2ChromaSync.log)
├── GameState.h / .cpp       # ScriptHookRDR2 native calls (health, wanted, dead eye)
├── EventDetector.h / .cpp   # [Phase 1 legacy] Direct event detection
├── ChromaClient.h / .cpp    # WinHTTP client for Chroma REST API
│
├── core/                    # Foundation types
│   ├── Color.h              # RGBA color with lerp, blend, BGR conversion
│   ├── Easing.h             # 5 easing functions (Linear, EaseIn/Out/InOut, Sine)
│   ├── AnimationTimer.h     # Delta-time progress tracker
│   └── Framebuffer.h / .cpp # 6×22 per-key color grid
│
├── events/                  # Decoupled event system
│   ├── EventTypes.h         # Event enum + data struct
│   ├── EventBus.h / .cpp    # Pub/sub queue with per-type and global subscribers
│   └── GameStateMachine.h / .cpp  # IDLE/COMBAT/DEAD_EYE transitions with debounce
│
├── effects/                 # Visual effect library
│   ├── Effect.h             # Base interface (Start/Stop/Update/Render)
│   ├── StaticEffect.h       # Solid color fill
│   ├── PulseEffect.h        # Sine-wave brightness oscillation
│   ├── FlashEffect.h        # Short burst with eased fade-out
│   ├── BreathingEffect.h    # Slow in/out ambient cycle
│   ├── TransitionEffect.h   # Color A → B interpolation
│   ├── RippleEffect.h       # Expanding ring from center
│   └── EffectRegistry.h / .cpp  # Declarative event → effect mapping
│
└── render/                  # Output pipeline
    ├── Layer.h              # Named layer with blend mode + active effect
    ├── LayerStack.h         # 6 predefined layers (BASE → DEBUG)
    ├── Compositor.h / .cpp  # Per-pixel layer blending
    └── ChromaRenderer.h / .cpp  # Framebuffer → CHROMA_CUSTOM REST payload
```

---

## Core Concepts

### Framebuffer

A `6×22` grid of `Color` pixels representing the Razer keyboard layout. Effects write to framebuffers, and the compositor merges them into a final output buffer.

```cpp
Framebuffer fb;
fb.SetPixel(2, 10, Color::Red());  // Row 2, Column 10
fb.Fill(Color::Black());           // Clear everything
```

### Color

RGBA internally (0–255 per channel). Converts to BGR integer for the Chroma REST API.

```cpp
Color c(255, 128, 0);           // Orange (RGB)
Color lerped = Color::Lerp(Color::Red(), Color::Blue(), 0.5f);  // Purple
int bgr = c.ToBGR();            // For Chroma API
Color blended = Color::Blend(base, overlay, BlendMode::ALPHA_BLEND);
```

### Events

Events flow through an `EventBus` using pub/sub:

| Event | Trigger |
|-------|---------|
| `WANTED_STARTED` | Wanted level goes from 0 → 1+ |
| `WANTED_CLEARED` | Wanted level drops to 0 |
| `DEAD_EYE_ACTIVATED` | Frame time drops (slow-mo detected) |
| `DEAD_EYE_DEACTIVATED` | Frame time returns to normal |
| `LOW_HEALTH_ENTERED` | Health drops below 25% |
| `LOW_HEALTH_EXITED` | Health recovers above 25% |
| `STATE_IDLE/COMBAT/DEAD_EYE` | High-level state transitions |

### Effects

All effects inherit from `Effect` and implement:

```cpp
void Start();                    // Called when activated
void Stop();                     // Called when deactivated
void Update(float deltaTime);    // Called every frame (seconds)
void Render(Framebuffer& target);// Write pixels to the framebuffer
bool IsFinished();               // True = auto-cleanup
```

Effects **never** talk to the Chroma SDK directly. They only write to framebuffers.

### Layers

Six layers in priority order (lower = rendered first):

| Layer | Priority | Blend Mode | Purpose |
|-------|----------|------------|---------|
| `BASE` | 0 | REPLACE | Idle breathing / default color |
| `AMBIENT` | 10 | ALPHA_BLEND | Environmental effects (unused) |
| `WEATHER` | 20 | ALPHA_BLEND | Weather effects (unused) |
| `COMBAT` | 30 | REPLACE | Wanted pulse / low health |
| `CRITICAL` | 40 | REPLACE | Dead Eye breathing |
| `DEBUG` | 50 | REPLACE | Debug visualization (unused) |

### Compositor

Blends all active layers into the final framebuffer using each layer's blend mode:

```
Final pixel = blend(blend(blend(BASE, AMBIENT), COMBAT), CRITICAL)
```

Inactive layers are skipped. The compositor runs every frame.

### ChromaRenderer

Converts the final `Framebuffer` to a `CHROMA_CUSTOM` JSON payload and sends it via `PUT /keyboard`:

```json
{
  "effect": "CHROMA_CUSTOM",
  "param": [[0, 0, ...], [0, 0, ...], ...]   // 6 rows × 22 columns of BGR ints
}
```

**Frame diffing**: only sends if pixels changed from the last frame.

---

## Main Render Loop

Located in `dllmain.cpp → ScriptMain()`:

```
while (true) {
    1. Calculate delta time (wall clock, NOT game time)
    2. Ensure Chroma session is alive (auto-reconnect)
    3. Send heartbeat (every 1s)
    4. Read game state via ScriptHookRDR2 natives
    5. Update GameStateMachine → emits events to EventBus
    6. Process event queue → EffectRegistry activates/deactivates effects
    7. Update all active effects (deltaTime)
    8. Render effects into their layer framebuffers
    9. Composite layers into final framebuffer
   10. Push to hardware via ChromaRenderer
   11. Log stats every 5 seconds
   12. WAIT(33)  — yield to game engine (~30 FPS)
}
```

**Critical rule**: `WAIT()` must be reached every iteration. It yields the ScriptHook fiber back to the game engine. Skipping it freezes the game.

---

## Configuration

All tunable values live in `RDR2ChromaSync_config.json` (placed next to the `.asi`):

```json
{
  "timing": { "render_interval_ms": 33, "warmup_frames": 30 },
  "effects": {
    "pulse":     { "frequency_hz": 2.0, "min_brightness": 0.3 },
    "breathing": { "cycle_seconds": 3.0, "min_brightness": 0.4 }
  },
  "colors": {
    "idle":      { "r": 64,  "g": 64,  "b": 64  },
    "wanted":    { "r": 255, "g": 0,   "b": 0   },
    "dead_eye":  { "r": 128, "g": 85,  "b": 45  }
  }
}
```

Falls back to hardcoded defaults if the file is missing.

---

## Adding a New Effect

1. Create `src/effects/MyEffect.h` inheriting from `Effect`
2. Implement `Start()`, `Stop()`, `Update(dt)`, `Render(fb)`, `IsFinished()`
3. Register it in `dllmain.cpp → RegisterEffects()`:
   ```cpp
   registry.Register(EventType::SOME_EVENT, "COMBAT", []() -> Effect* {
       return new MyEffect(Color::Red(), 2.0f);
   });
   ```
4. Add it to `RDR2ChromaSync.vcxproj` `<ClInclude>` section
5. Build → deploy

---

## Adding a New Event

1. Add the event to `EventTypes.h`:
   ```cpp
   enum class EventType { ..., MY_NEW_EVENT, ... };
   ```
2. Update `EventTypeName()` in the same file
3. Emit it from `GameStateMachine.cpp`:
   ```cpp
   bus.Emit(EventType::MY_NEW_EVENT, someValue);
   ```
4. Register an effect for it in `dllmain.cpp → RegisterEffects()`

---

## Build & Deploy

```bash
# Build (Visual Studio 2026, v145 toolset)
MSBuild RDR2ChromaSync.vcxproj /p:Configuration=Release /p:Platform=x64

# Deploy (copy to game directory)
copy build\Release\RDR2ChromaSync.asi "I:\Games\Red Dead Redemption 2\"
copy config.json "I:\Games\Red Dead Redemption 2\RDR2ChromaSync_config.json"
```

### Requirements
- Visual Studio 2026 (v145 toolset) or compatible MSVC
- Windows SDK 10.0
- ScriptHookRDR2 SDK (included in `include/`)
- nlohmann/json (included in `include/nlohmann/`)
- Razer Synapse 3 with Chroma Connect enabled

---

## Logging

All systems log to `RDR2ChromaSync.log` next to the game executable.

| Level | When |
|-------|------|
| `INFO` | Events, state transitions, effect activation, initialization |
| `DEBUG` | Frame sends, render timing, periodic stats (every 5s) |
| `ERROR` | HTTP failures, missing layers, exceptions |

Useful grep patterns for debugging:
```bash
# See all state transitions
grep "GameStateMachine:" RDR2ChromaSync.log

# See all effect activations
grep "EffectRegistry:" RDR2ChromaSync.log

# See render stats
grep "STATS" RDR2ChromaSync.log

# See errors only
grep "ERROR" RDR2ChromaSync.log
```

---

## Constraints

- **Single-threaded**: everything runs on the ScriptHook fiber. No mutexes needed.
- **No blocking calls**: HTTP timeouts are 2 seconds max. `WAIT()` must always be reached.
- **No game file modification**: only `RDR2ChromaSync.asi` and `RDR2ChromaSync_config.json` are deployed. No other game files may be touched.
- **Dead Eye detection**: uses `GET_FRAME_TIME()` slowdown heuristic (the direct native crashes the game).
