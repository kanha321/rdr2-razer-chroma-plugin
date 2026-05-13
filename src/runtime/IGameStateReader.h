/*
 * IGameStateReader.h — Game state source abstraction
 */

#pragma once

#include "../GameState.h"

class IGameStateReader
{
public:
    virtual ~IGameStateReader() = default;
    virtual GameState Read() = 0;
};
