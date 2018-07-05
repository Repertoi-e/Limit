#include "TestGame_Tile.h"

#include "../Intrinsics.h"

inline TileChunk* GetTileChunk(TileMap *tileMap, int chunkX, int chunkY)
{
	TileChunk *chunk = 0;
	if ((chunkX >= 0) && (chunkX < tileMap->TileChunkCountX) &&
		(chunkY >= 0) && (chunkY < tileMap->TileChunkCountY))
		chunk = &tileMap->TileChunks[chunkY * tileMap->TileChunkCountX + chunkX];
	return chunk;
}

inline byte GetTileValueUnchecked(TileMap *tileMap, TileChunk *chunk, u32 x, u32 y)
{
	Assert(chunk);
	Assert(x < tileMap->ChunkDim);
	Assert(y < tileMap->ChunkDim);
	
	return chunk->Tiles[y * tileMap->ChunkDim + x];
}

inline void SetTileValueUnchecked(TileMap *tileMap, TileChunk *chunk, u32 x, u32 y, byte value)
{
	Assert(chunk);
	Assert(x < tileMap->ChunkDim);
	Assert(y < tileMap->ChunkDim);
	
	chunk->Tiles[y * tileMap->ChunkDim + x] = value;
}

inline byte GetTileValue(TileMap *tileMap, TileChunk *chunk, u32 x, u32 y)
{
	
	byte value = 0;
	if (chunk)
		value = GetTileValueUnchecked(tileMap, chunk, x, y);
	return value;
}

inline void SetTileValue(TileMap *tileMap, TileChunk *chunk, u32 x, u32 y, byte value)
{
	if (chunk)
		SetTileValueUnchecked(tileMap, chunk, x, y, value);
}

inline TileChunkPosition GetChunkPositionFor(TileMap *tileMap, u32 absTileX, u32 absTileY)
{
	TileChunkPosition result;
	result.TileChunkX = absTileX >> tileMap->ChunkShift;
	result.TileChunkY = absTileY >> tileMap->ChunkShift;
	result.TileRelX = absTileX & tileMap->ChunkMask;
	result.TileRelY = absTileY & tileMap->ChunkMask;
	
	return result;
}


inline void CanonizeCoord(TileMap *tileMap, u32 *tile, real32 *tileRel)
{
	// The world is assumed to be toroidal topology, if you step off one you come back in the other
	int offset = RoundToS32(*tileRel / tileMap->TileSideInMeters);
	*tile += offset;
	*tileRel -= offset * tileMap->TileSideInMeters;
	
	// Round to 5 decimal places to reduce floating point errors causing lines to appear when drawing.
	*tileRel = std::roundf(*tileRel * 100000.f) / 100000.f;
	
	Assert(*tileRel >= -.5f * tileMap->TileSideInMeters);
	Assert(*tileRel <= .5f * tileMap->TileSideInMeters);
}

inline TileMapPosition CanonizePosition(TileMap *tileMap, TileMapPosition position)
{
	TileMapPosition result = position;
	
	CanonizeCoord(tileMap, &result.AbsTileX, &result.TileRelX);
	CanonizeCoord(tileMap, &result.AbsTileY, &result.TileRelY);
	
	return result;
}

static byte GetTileValue(TileMap *tileMap, u32 absTileX, u32 absTileY)
{
	TileChunkPosition chunkPosition = GetChunkPositionFor(tileMap, absTileX, absTileY);
	TileChunk *chunk = GetTileChunk(tileMap, chunkPosition.TileChunkX, chunkPosition.TileChunkY);
	return GetTileValue(tileMap, chunk, chunkPosition.TileRelX, chunkPosition.TileRelY);
}

// #TODO: Support on-demand tile chunk creation
static void SetTileValue(TileMap *tileMap, u32 absTileX, u32 absTileY, byte value)
{
	TileChunkPosition chunkPosition = GetChunkPositionFor(tileMap, absTileX, absTileY);
	TileChunk *chunk = GetTileChunk(tileMap, chunkPosition.TileChunkX, chunkPosition.TileChunkY);
	
	if (chunk)
		SetTileValue(tileMap, chunk, chunkPosition.TileRelX, chunkPosition.TileRelY, value);
}

inline bool32 IsWorldPointEmpty(TileMap *tileMap, TileMapPosition position)
{
	return GetTileValue(tileMap, position.AbsTileX, position.AbsTileY) == 0;
}

inline TileMapPosition ScreenCoordsToTileMapPosition(const GameOffscreenBuffer& screenBuffer, TileMap *tileMap, TileMapPosition cameraPosition, int screenX, int screenY)
{
	TileMapPosition result;
	
	real32 screenCenterX = (real32) screenBuffer.Width  / 2;
	real32 screenCenterY = (real32) screenBuffer.Height / 2;
	
	real32 offsetX = (real32) (screenX - screenCenterX + cameraPosition.TileRelX * tileMap->MetersToPixels + tileMap->TileSideInPixels / 2);
	real32 offsetY = (real32) (screenCenterY - screenY + cameraPosition.TileRelY * tileMap->MetersToPixels + tileMap->TileSideInPixels / 2);
	
	int tileX = (int) (offsetX / tileMap->TileSideInPixels);
	int tileY = (int) (offsetY / tileMap->TileSideInPixels);
	
	result.AbsTileX = tileX + cameraPosition.AbsTileX;
	result.AbsTileY = tileY + cameraPosition.AbsTileY;
	
	result.TileRelX = offsetX - tileX * tileMap->TileSideInPixels;
	result.TileRelY = offsetY - tileY * tileMap->TileSideInPixels;
	
	return result;
}