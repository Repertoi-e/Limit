#pragma once

#include "../Limit.h"

#include "TestGame_Tile.h"

struct MemoryArena
{
	byte*Base;
	u64 Used;
	u64 Size;
};

struct GameState
{
	World *World;
	TileMapPosition PlayerPosition;
	
	bool32 IsInitialized;
};

