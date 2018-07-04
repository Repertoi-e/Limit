#pragma once

#include "../Limit.h"

struct TileChunk
{
	byte *Tiles;
};

struct TileMap
{
	u32 ChunkShift;
	u32 ChunkMask;
	u32 ChunkDim;

	real32 TileSideInMeters;
	int TileSideInPixels;
	real32 MetersToPixels;

	real32 LowerLeftX;
	real32 LowerLeftY;

	int TileChunkCountX;
	int TileChunkCountY;
	TileChunk *TileChunks;
};

struct World
{
	TileMap *TileMap;
};

struct WorldPosition
{
	u32 AbsTileX;
	u32 AbsTileY;

	// X and Y relative to the tile we are in in this tilemap
	real32 TileRelX;
	real32 TileRelY;
};
