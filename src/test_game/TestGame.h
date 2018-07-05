#pragma once

#include "../Limit.h"

#include "TestGame_Tile.h"

struct GameState
{
	World *World;
	TileMapPosition PlayerPosition;
	
	bool32 IsInitialized;
};

