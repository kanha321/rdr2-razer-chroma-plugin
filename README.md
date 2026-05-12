# RDR2ChromaSync

**Razer Chroma RGB event lighting for Red Dead Redemption 2**

An ASI plugin that detects in-game events (Wanted Level, Low Health, Dead Eye) and translates them into dynamic keyboard lighting via the Razer Chroma REST API.

---

## Features (Phase 1)

| Event | Trigger | Keyboard Color |
|---|---|---|
| 🔴 Wanted | Wanted level ≥ 1 star | Red |
| 🟠 Low Health | Health below 25% | Orange |
| 🟤 Dead Eye | Dead Eye ability active | Sepia/Amber |
| ⚪ Idle | No active event | Dim White |

**Priority:** Dead Eye → Wanted → Low Health → Idle

---

## Prerequisites

- **Red Dead Redemption 2** (Story Mode)
- **ScriptHookRDR2** — Download from [dev-c.com](http://www.dev-c.com/rdr2/scripthookrdr2/)
- **Razer Synapse 3** — Running with Chroma Connect enabled
- **Visual Studio 2022** — With C++ Desktop Development workload

---

## Setup & Build

### 1. Download ScriptHookRDR2 SDK

Download the SDK from [dev-c.com/rdr2/scripthookrdr2](http://www.dev-c.com/rdr2/scripthookrdr2/) and:

- Copy the **header files** (`main.h`, `types.h`, `natives.h`, `enums.h`) from the SDK's `inc/` folder into:
  ```
  include/ScriptHookRDR2/
  ```
  These will **replace** the stub headers provided in this repo.

- Copy `ScriptHookRDR2.lib` from the SDK's `lib/` folder into:
  ```
  lib/
  ```

### 2. Build

1. Open `RDR2ChromaSync.sln` in Visual Studio 2022
2. Select **Release | x64** configuration
3. Build → Build Solution (Ctrl+Shift+B)
4. Output: `build/Release/RDR2ChromaSync.asi`

### 3. Install

Copy the following files to your RDR2 game directory:

```
<game_root>/
├── RDR2ChromaSync.asi          ← Your built plugin
├── ScriptHookRDR2.dll          ← From ScriptHookRDR2 download
└── dinput8.dll                 ← ASI Loader (from ScriptHookRDR2 download)
```

**Game directory locations:**
- Rockstar Launcher: `C:\Program Files\Rockstar Games\Red Dead Redemption 2\`
- Steam: `C:\Program Files (x86)\Steam\steamapps\common\Red Dead Redemption 2\`

### 4. Run

1. Make sure **Razer Synapse 3** is running
2. Launch RDR2 (Story Mode)
3. The plugin loads automatically — your keyboard will light up!

---

## How It Works

```
Game Launch → ASI Loader loads RDR2ChromaSync.asi
                    │
                    ▼
            ScriptMain() starts
                    │
                    ▼
        ChromaClient::Initialize()
        → POST to Chroma SDK, get session URI
                    │
                    ▼
            ┌─── Main Loop (every ~100ms) ───┐
            │                                 │
            │  1. Heartbeat (every 1s)        │
            │  2. Read game state (natives)   │
            │  3. Detect priority event       │
            │  4. If changed → PUT color      │
            │  5. WAIT(100) → repeat          │
            │                                 │
            └─────────────────────────────────┘
                    │
                    ▼
            On exit: DELETE session
```

---

## Project Structure

```
RDR2ChromaSync/
├── src/
│   ├── dllmain.cpp          # ASI entry point + main loop
│   ├── GameState.h/cpp      # Reads RDR2 native values
│   ├── EventDetector.h/cpp  # Priority-based event detection
│   ├── ChromaClient.h/cpp   # WinHTTP Chroma REST client
│   └── Config.h             # Constants, colors, thresholds
├── include/
│   ├── ScriptHookRDR2/      # SDK headers (stubs → replace with real)
│   └── nlohmann/json.hpp    # Header-only JSON library
├── lib/                     # ScriptHookRDR2.lib (user-supplied)
├── references/              # Cloned reference projects
├── RDR2ChromaSync.vcxproj   # VS 2022 project
├── RDR2ChromaSync.sln       # VS solution
└── README.md
```

---

## Configuration

Edit `src/Config.h` to customize:

- **Colors** — Change BGR values for each event
- **Health threshold** — Adjust when low health triggers (default: 25%)
- **Polling rate** — Change main loop interval (default: 100ms)
- **Heartbeat interval** — Chroma keepalive timing (default: 1000ms)

---

## Troubleshooting

| Issue | Solution |
|---|---|
| Keyboard doesn't change color | Ensure Razer Synapse 3 is running with Chroma Connect enabled |
| Plugin doesn't load | Verify `ScriptHookRDR2.dll` and `dinput8.dll` are in the game directory |
| Game crashes on launch | Replace stub headers with real ScriptHookRDR2 SDK files |
| Colors don't reset on exit | Reopen Synapse or restart the Chroma service |
| Build error: LNK1181 | Place `ScriptHookRDR2.lib` in the `lib/` directory |

---

## Credits

- **ScriptHookRDR2** by Alexander Blade — [dev-c.com](http://www.dev-c.com/rdr2/scripthookrdr2/)
- **nlohmann/json** — [github.com/nlohmann/json](https://github.com/nlohmann/json)
- **RGB4R** by Lemon — [github.com/justalemon/RGB4R](https://github.com/justalemon/RGB4R) (architecture reference)
- **Razer Chroma SDK** — [developer.razer.com](https://developer.razer.com/chroma/)

---

## License

This project is provided as-is for educational and personal use. ScriptHookRDR2 SDK files are not included and must be obtained separately from the official source.
