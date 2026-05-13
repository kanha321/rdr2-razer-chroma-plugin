/*
 * DefaultGameStateReader.h — Default ScriptHook-backed reader
 */

#pragma once

#include "IGameStateReader.h"

class DefaultGameStateReader : public IGameStateReader
{
public:
    GameState Read() override
    {
        return GameState::Read();
    }
};
