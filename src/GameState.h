/*
 * GameState.h — Game state snapshot structure and reader
 *
 * Reads RDR2 native values each tick to produce a clean snapshot
 * that the EventDetector can evaluate without direct native calls.
 *
 * Pattern: RDR2Extension/script.cpp — PLAYER::PLAYER_ID(), PLAYER_PED_ID(),
 *          entity existence check before accessing any ped data.
 */

#pragma once

struct GameState
{
    // Whether the player ped is valid and controllable
    bool isPlayerValid;

    // Current wanted level (0-5)
    int wantedLevel;

    // Health values
    // RDR2 health range: 100 (dead) to maxHealth (typically 200)
    int currentHealth;
    int maxHealth;

    // Dead Eye state
    bool isDeadEyeActive;

    // Default constructor — safe zero state
    GameState()
        : isPlayerValid(false)
        , wantedLevel(0)
        , currentHealth(0)
        , maxHealth(0)
        , isDeadEyeActive(false)
    {
    }

    // Read all game state values from RDR2 natives
    // Returns a populated GameState snapshot
    static GameState Read();
};
