#include "TestGame_Tile.h"

#include "../Intrinsics.h"

inline TileChunk* GetTileChunk(TileMap *tileMap, int chunkX, int chunkY)
{
	chunkX = chunkX >> tileMap->ChunkShift;
	chunkY = chunkY >> tileMap->ChunkShift;
	
	TileChunk *chunk = 0;
	if ((chunkX >= 0) && (chunkX < tileMap->TileChunkCountX) &&
		(chunkY >= 0) && (chunkY < tileMap->TileChunkCountY))
		chunk = &tileMap->TileChunks[chunkY * tileMap->TileChunkCountX + chunkX];
	return chunk;
}

static byte GetTileValue(TileMap *tileMap, u32 absTileX, u32 absTileY)
{
	byte value = 0;
	TileChunk *chunk = GetTileChunk(tileMap, absTileX, absTileY);
	u32 relTileX = absTileX & tileMap->ChunkMask;
	u32 relTileY = absTileY & tileMap->ChunkMask;
	if (chunk)
		value = chunk->Tiles[relTileY * tileMap->ChunkDim + relTileX];
	return value;
}

static void SetTileValue(TileMap *tileMap, u32 absTileX, u32 absTileY, byte value)
{
	TileChunk *chunk = GetTileChunk(tileMap, absTileX, absTileY);
	u32 relTileX = absTileX & tileMap->ChunkMask;
	u32 relTileY = absTileY & tileMap->ChunkMask;
	if (chunk)
		chunk->Tiles[relTileY * tileMap->ChunkDim + relTileX] = value;
}


inline void CanonizeCoord(TileMap *tileMap, u32 *tile, real32 *tileRel)
{
	// The world is assumed to be toroidal topology, if you step off one you come back in the other
	int offset = RoundToS32((*tileRel) / tileMap->TileSideInMeters);
	*tile += offset;
	*tileRel -= offset * tileMap->TileSideInMeters;
	
	Assert(*tileRel >= -.5f * tileMap->TileSideInMeters);
	Assert(*tileRel <= .5f * tileMap->TileSideInMeters);
}

inline WorldPosition CanonizePosition(TileMap *tileMap, WorldPosition position)
{
	WorldPosition result = position;
	
	CanonizeCoord(tileMap, &result.AbsTileX, &result.TileRelX);
	CanonizeCoord(tileMap, &result.AbsTileY, &result.TileRelY);
	
	return result;
}

inline bool32 IsWorldPointEmpty(TileMap *tileMap, WorldPosition position)
{
	return GetTileValue(tileMap, position.AbsTileX, position.AbsTileY) == 0;
}