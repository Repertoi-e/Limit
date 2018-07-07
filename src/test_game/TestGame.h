#pragma once

#include "../Limit.h"

#include "TestGame_Tile.h"

struct LoadedBMP
{
	int Width, Height;
	u32 *Pixels;
};

struct GameState
{
	World *World;
	TileMapPosition PlayerPosition;
	
	LoadedBMP Test, Background, Player;
	
	bool32 IsInitialized;
};

