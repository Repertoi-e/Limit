#include "TestGame.h"

#include "TestGame_Tile.cpp"

#include "../Intrinsics.h"

static const GameMemory* g_Memory; // global for the game .dll

#define PushStruct(Arena, Type) (Type *)PushSize_(Arena, sizeof(Type))
#define PushArray(Arena, Count, Type) (Type *)PushSize_(Arena, (Count) * sizeof(Type))

static byte* PushSize_(MemoryArena *arena, u32 size)
{
	Assert(arena->Used + size <= arena->Size);
	byte *result = arena->Base + arena->Used;
	arena->Used += size;
	return result;
}

static u32 CountCharacter(Char *string, Char ch)
{
	if (ch == TEXT('\0'))
		return 1; // technically there is one null terminator in every valid string
	
	u32 result = 0;
	while (true)
	{
		Char now = *string++;
		if (now == ch)
			++result;
		else if (now == TEXT('\0'))
			break;
	}
	return result;
}

static void SaveTilemapToSrcFile()
{
	/*// #TODO Unfinished
 auto [srcBytes, srcLen] = g_Memory->DEBUGPlatformReadEntireFile(TEXT(__FILE__));
 
 Char *src = (Char *) srcBytes;
 
 Char newLine = TEXT('\n');
 u32 lines = CountCharacter(src, newLine);
 
 Char log[30];
 _stprintf_s(log, 30, TEXT("%d\n"), lines);
 OutputDebugString(log);
 */// #TODO Unfinished
}

static void DrawRectangle(GameOffscreenBuffer *buffer, real32 fMinX, real32 fMinY, real32 fMaxX, real32 fMaxY, real32 r, real32 g, real32 b)
{
	int minX = RoundToS32(fMinX);
	int minY = RoundToS32(fMinY);
	int maxX = RoundToS32(fMaxX);
	int maxY = RoundToS32(fMaxY);
	
	if (minX < 0)
		minX = 0;
	
	if (minY < 0)
		minY = 0;
	
	if (maxX > buffer->Width)
		maxX = buffer->Width;
	
	if (maxY > buffer->Height)
		maxY = buffer->Height;
	
	u32 color = (RoundToS32(r * 255.f) << 16) | (RoundToS32(g * 255.f) << 8) | RoundToS32(b * 255.f);
	
	byte *row = ((byte *) buffer->Memory + minX * buffer->BytesPerPixel + minY * buffer->Pitch);
	for (int y = minY; y < maxY; ++y)
	{
		u32 *pixel = (u32 *) row;
		for (int x = minX; x < maxX; ++x)
			*(u32 *) pixel++ = color;
		row += buffer->Pitch;
	}
}

EXPORT GAME_UPDATE_AND_RENDER(GameUpdateAndRender /*const GameMemory& gameMemory, const GameInput& input, GameOffscreenBuffer *screenBuffer*/)
{
	GameState *state = (GameState *) gameMemory.Permanent;
	
	// Get references to avoid having to type state-> everywhere (syntactic sugar)
	TileMapPosition& playerPosition = state->PlayerPosition;
	World*& world = state->World;
	
	if (!state->IsInitialized)
	{
		state->IsInitialized = true;
		
		/* Code that runs on game intitialization */
		g_Memory = &gameMemory;
		
		MemoryArena worldArena = {};
		worldArena.Base = (byte *) gameMemory.Permanent + sizeof(GameState);
		worldArena.Size = gameMemory.PermanentSize - sizeof(GameState);
		worldArena.Used = 0;
		
		world = PushStruct(&worldArena, World);
		
		TileMap *tileMap = PushStruct(&worldArena, TileMap);
		world->TileMap = tileMap;
		
		tileMap->ChunkShift = 4;
		tileMap->ChunkMask = (1 << tileMap->ChunkShift) - 1;
		tileMap->ChunkDim = 1 << tileMap->ChunkShift;
		
		tileMap->TileChunkCountX = 120;
		tileMap->TileChunkCountY = 120;
		
		tileMap->TileSideInMeters = 1.4f;
		tileMap->TileSideInPixels = 15;
		tileMap->MetersToPixels = tileMap->TileSideInPixels / tileMap->TileSideInMeters;
		
		
		playerPosition.AbsTileX = 3;
		playerPosition.AbsTileY = 3;
		playerPosition.TileRelX = 5.0f;
		playerPosition.TileRelY = 5.0f;
		playerPosition = CanonizePosition(tileMap, playerPosition);
		
		tileMap->TileChunks = PushArray(&worldArena, tileMap->TileChunkCountY * tileMap->TileChunkCountX, TileChunk);
		
		for (int y = 0; y < tileMap->TileChunkCountY; y++)
		{
			for (int x = 0; x < tileMap->TileChunkCountX; x++)
				tileMap->TileChunks[y * tileMap->TileChunkCountX + x].Tiles = PushArray(&worldArena, tileMap->ChunkDim * tileMap->ChunkDim, byte);
		}
		
		int screenXCount = 20;
		int screenYCount = 20;
		int tilePerWidth = 20;
		int tilePerHeight = 20;
		
		for (int sx = 0; sx < screenXCount; sx++)
		{
			for (int sy = 0; sy < screenYCount; sy++)
			{
				for (int x = 0; x < tilePerWidth; x++)
				{
					for (int y = 0; y < tilePerHeight; y++)
					{
						// byte value = (x == (y + 1)) ? 1 : 0;
						byte value = ((x + y) % 2 == 0) && (rand() % 7 == 0) ? 1 : 0;
						SetTileValue(tileMap, sx * screenXCount + x, sy * screenYCount + y, value);
					}
				}
			}
		}
		
	}
	
	TileMap* tileMap = world->TileMap;
	
	const real32 playerWidth = tileMap->TileSideInMeters * .75f;
	const real32 playerHeight = (real32) tileMap->TileSideInMeters;
	
	/* Handle input */
	{
		if (input.MouseButtons[MOUSE_LEFT].EndedDown)
		{
			int tileX = input.MouseX / tileMap->TileSideInPixels;
			int tileY = input.MouseY / tileMap->TileSideInPixels;
			
			//currentTileMap->Tiles[tileY * world.TileMapWidth + tileX] = 1;
			
			SaveTilemapToSrcFile();
		}
		
		if (input.MouseButtons[MOUSE_RIGHT].EndedDown)
		{
			int tileX = input.MouseX / tileMap->TileSideInPixels;
			int tileY = input.MouseY / tileMap->TileSideInPixels;
			
			//currentTileMap->Tiles[tileY * world.TileMapWidth + tileX] = 0;
			
			SaveTilemapToSrcFile();
		}
		
		int dx = 0, dy = 0;
		if (input.MoveUp.EndedDown)
			dy += 1;
		if (input.MoveDown.EndedDown)
			dy -= 1;
		if (input.MoveLeft.EndedDown)
			dx -= 1;
		if (input.MoveRight.EndedDown)
			dx += 1;
		
		int speed = 5; // metres/second
		if (input.Sprint.EndedDown)
			speed *= 10;
		
		TileMapPosition newPlayerPosition = playerPosition;
		newPlayerPosition.TileRelX += input.DeltaTime * dx * speed;
		newPlayerPosition.TileRelY += input.DeltaTime * dy * speed;
		newPlayerPosition = CanonizePosition(tileMap, newPlayerPosition);
		
		TileMapPosition playerLeft = newPlayerPosition;
		playerLeft.TileRelX -= 0.5f * playerWidth;
		playerLeft = CanonizePosition(tileMap, playerLeft);
		TileMapPosition playerRight = newPlayerPosition;
		playerRight.TileRelX += 0.5f * playerWidth;
		playerRight = CanonizePosition(tileMap, playerRight);
		
		
		if (IsWorldPointEmpty(tileMap, newPlayerPosition) && IsWorldPointEmpty(tileMap, playerLeft) && IsWorldPointEmpty(tileMap, playerRight))
		{
			playerPosition = newPlayerPosition;
		}
	}
	
	/* Render */
	DrawRectangle(screenBuffer, .0f, .0f, (real32) screenBuffer->Width, (real32) screenBuffer->Height, .0f, .0f, .0f);
	
	// Draw the world
	real32 screenCenterX = .5f * (real32) screenBuffer->Width;
	real32 screenCenterY = .5f * (real32) screenBuffer->Height;
	
	for (int relRow = -20; relRow < 20; ++relRow)
	{
		for (int relCol = -30; relCol < 30; ++relCol)
		{
			u32 col = playerPosition.AbsTileX + relCol;
			u32 row = playerPosition.AbsTileY + relRow;
			
			real32 gray = 0.5f;
			if (GetTileValue(tileMap, col, row) == 1)
				gray = 1.0f;
			if ((col == playerPosition.AbsTileX) && (row == playerPosition.AbsTileY))
				gray = 0;
			
			real32 centerX = screenCenterX - tileMap->MetersToPixels * playerPosition.TileRelX + (real32) relCol * tileMap->TileSideInPixels;
			real32 centerY = screenCenterY + tileMap->MetersToPixels * playerPosition.TileRelY - (real32) relRow * tileMap->TileSideInPixels;
			
			
			real32 minX = centerX - .5f * tileMap->TileSideInPixels;
			real32 minY = centerY - .5f * tileMap->TileSideInPixels;
			real32 maxX = centerX + .5f * tileMap->TileSideInPixels;
			real32 maxY = centerY + .5f * tileMap->TileSideInPixels;
			
			DrawRectangle(screenBuffer, minX, minY, maxX, maxY, gray, gray, gray);
		}
	}
	
	// Draw the player
	
	real32 playerMinX = screenCenterX - 0.5f * tileMap->MetersToPixels * playerWidth;
	real32 playerMinY = screenCenterY - tileMap->MetersToPixels * playerHeight;
	
	DrawRectangle(screenBuffer, playerMinX, playerMinY, playerMinX + tileMap->MetersToPixels * playerWidth, playerMinY + tileMap->MetersToPixels * playerHeight, .2f, .3f, .8f);
}


// Outputs sound for the current frame
EXPORT GAME_GET_SOUND_SAMPLES(GameGetSoundSamples /*const GameMemory& gameMemory, GameSoundOutputBuffer *soundOutput*/)
{}
