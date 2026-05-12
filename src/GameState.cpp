/*
 * GameState.cpp — Reads RDR2 native values each tick
 *
 * This is the ONLY file that makes ScriptHookRDR2 native calls.
 * All other modules work with the GameState struct.
 *
 * Uses the REAL ScriptHookRDR2 SDK natives.h which provides all
 * native function wrappers in PLAYER:: and ENTITY:: namespaces.
 *
 * Safety pattern from RDR2Extension/script.cpp:L10-L12:
 *   - Check DOES_ENTITY_EXIST before accessing ped data
 *   - Check IS_PLAYER_CONTROL_ON to avoid reading during cutscenes
 */

#include "GameState.h"

// Real ScriptHookRDR2 SDK headers
#include "natives.h"
#include "types.h"
#include "main.h"
#include "nativeCaller.h"

GameState GameState::Read()
{
    GameState state;

    // Get player handles
    Player player = PLAYER::PLAYER_ID();
    Ped playerPed = PLAYER::PLAYER_PED_ID();

    // Safety check: entity must exist and player must have control
    // This pattern is directly from RDR2Extension/script.cpp:L11
    // Without this check, native calls can return garbage or crash
    if (!ENTITY::DOES_ENTITY_EXIST(playerPed) || !PLAYER::IS_PLAYER_CONTROL_ON(player))
    {
        state.isPlayerValid = false;
        return state;
    }

    state.isPlayerValid = true;

    // Wanted level (0-5)
    state.wantedLevel = PLAYER::GET_PLAYER_WANTED_LEVEL(player);

    // Health values
    // RDR2 health: 100 = dead, max is typically 200
    // Used by EventDetector with formula: (current - 100) / (max - 100) < threshold
    state.currentHealth = ENTITY::GET_ENTITY_HEALTH(playerPed);

    // Real SDK: GET_ENTITY_MAX_HEALTH takes (entity, p1) — pass FALSE for p1
    state.maxHealth = ENTITY::GET_ENTITY_MAX_HEALTH(playerPed, FALSE);

    // Dead Eye active state
    // IMPORTANT: The native IS_PLAYER_DEAD_EYE_ENABLED (hash 0x7179619572B10D70)
    // does NOT exist in ScriptHookRDR2 SDK v1.0.1491.17 — calling it causes
    // FATAL: "Can't find native" and freezes the game.
    //
    // Workaround: Use GET_FRAME_TIME() to detect time slowdown.
    // Normal gameplay: frame time ~0.033s (30fps) or ~0.016s (60fps)
    // Dead Eye: time is slowed significantly, but GET_FRAME_TIME returns
    // the game's internal time, not real time. When Dead Eye activates,
    // the game's time scale drops to ~0.3x.
    //
    // We detect this by checking if frame time is abnormally LOW
    // (less than half of what 30fps would give), indicating time slowdown.
    // This is a well-known community workaround.
    float frameTime = GAMEPLAY::GET_FRAME_TIME();
    state.isDeadEyeActive = (frameTime > 0.0f && frameTime < 0.008f);

    return state;
}
