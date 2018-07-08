#include "TestGame.h"

#include "TestGame_Tile.cpp"

#include "../Intrinsics.h"

static const GameMemory* g_Memory; // global for the game .dll

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


#pragma pack(push, 1)
struct BMPHeader
{
	u16 FileType;
	u32 FileSize;
	u16 Reserved1, Reserved2;
	u32 BitmapOffset;
	u32 Size;
	int Width, Height;
	u16 Planes;
	u16 BitsPerPixel;
	u32 Compression;
	u32 SizeOfBitmap;
	int HorzResolution, VertResolution;
	u32 ColorsUsed;
	u32 ColorsImportant;
	
	u32 RedMask, GreenMask, BlueMask;
};
#pragma pack(pop)

static LoadedBMP DEBUGLoadBMP(DEBUGPlatformReadEntireFileFunc readFileProc, const Char *path)
{
	LoadedBMP result;
	ZeroMemory(&result, sizeof(LoadedBMP));
	{
		auto [contents, contentsSize] = readFileProc(path);
		if (contentsSize != 0)
		{
			BMPHeader *header = (BMPHeader *) contents;
			result.Width = header->Width;
			result.Height = header->Height;
			
			Assert(header->Compression == 3);
			
			u32 *pixels = (u32 *)((byte *) contents + header->BitmapOffset);
			result.Pixels = pixels;
			
			u32 alphaMask = ~(header->RedMask | header->GreenMask | header->BlueMask);
			
			auto [redShiftFound, redShift] = FindLeastSignificantSetBit(header->RedMask);
			auto [greenShiftFound, greenShift] = FindLeastSignificantSetBit(header->GreenMask);
			auto [blueShiftFound, blueShift] = FindLeastSignificantSetBit(header->BlueMask);
			auto [alphaShiftFound, alphaShift] = FindLeastSignificantSetBit(alphaMask);
			Assert(redShiftFound);
			Assert(greenShiftFound);
			Assert(blueShiftFound);
			Assert(alphaShiftFound);
			
			
			u32 *sourceDest = pixels;
			for (int y = 0; y < header->Height; ++y)
			{
				for (int x = 0; x < header->Width; ++x)
				{
					u32 color = *sourceDest;
					*sourceDest++ = (((color >> alphaShift) & 0xFF) << 24) | (((color >> redShift) & 0xFF) << 16) | (((color >> greenShift) & 0xFF) << 8) | ((color >> blueShift) & 0xFF);
				}
			}
		}
	}
	return result;
}

static void DrawBMP(GameOffscreenBuffer *buffer, const LoadedBMP& bmp, real32 realX, real32 realY)
{
	int minX = RoundToS32(realX);
	int minY = RoundToS32(realY);
	int maxX = RoundToS32(realX + bmp.Width);
	int maxY = RoundToS32(realY + bmp.Height);
	
	int offsetX = 0;
	if (minX < 0)
	{
		offsetX = -minX;
		minX = 0;
	}
	int offsetY = 0;
	if (minY < 0)
	{
		offsetY = -minY;
		minY = 0;
	}
	if (maxX > buffer->Width)
		maxX = buffer->Width;
	if (maxY > buffer->Height)
		maxY = buffer->Height;
	
	u32 *sourceRow = bmp.Pixels + bmp.Width * (bmp.Height - 1);
	sourceRow += -offsetY * bmp.Width + offsetX;
	byte *destRow = ((byte *) buffer->Memory + minX * buffer->BytesPerPixel + minY * buffer->Pitch);
	for (int y = minY; y < maxY; ++y)
	{
		u32 *dest = (u32 *) destRow;
		u32 *source = sourceRow;
		for (int x = minX; x < maxX; ++x)
		{
			real32 a = (real32)((*source >> 24) & 0xFF) / 255.0f;
			real32 sr = (real32)((*source >> 16) & 0xFF);
			real32 sg = (real32)((*source >> 8) & 0xFF);
			real32 sb = (real32)(*source & 0xFF);
			
			real32 dr = (real32)((*dest >> 16) & 0xFF);
			real32 dg = (real32)((*dest >> 8) & 0xFF);
			real32 db = (real32)(*dest & 0xFF);
			
			real32 r = (1.f - a) * dr + a * sr;
			real32 g = (1.f - a) * dg + a * sg;
			real32 b = (1.f - a) * db + a * sb;
			
			*dest = RoundToU32(r) << 16 | RoundToU32(g) << 8 | RoundToU32(b);
			
			++dest;
			++source;
		}
		destRow += buffer->Pitch;
		sourceRow -= bmp.Width;
	}
}

EXPORT GAME_UPDATE_AND_RENDER(GameUpdateAndRender /*const GameMemory& gameMemory, const GameInput& input, GameOffscreenBuffer *screenBuffer*/)
{
	GameState *state = (GameState *) gameMemory.Permanent;
	
	// Get references to avoid having to type state-> everywhere (syntactic sugar)
	TileMapPosition& playerPosition = state->PlayerPosition;
	World*& world = state->World;
	
	constexpr size_t LogMessageSize = 512;
	Char LogMessage[LogMessageSize]; // Buffer used for _stprintf_s-ing text to the VS Debug output
	
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
		tileMap->TileSideInPixels = 30;
		tileMap->MetersToPixels = tileMap->TileSideInPixels / tileMap->TileSideInMeters;
		
		
		playerPosition.AbsTileX = 3;
		playerPosition.AbsTileY = 3;
		playerPosition.TileRelX = 5.0f;
		playerPosition.TileRelY = 5.0f;
		
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
						byte value = ((x + y) % 2 == 0) && (rand() % 70 == 0) ? 1 : 0;
						SetTileValue(tileMap, sx * screenXCount + x, sy * screenYCount + y, value);
					}
				}
			}
		}
		
		
		state->Test = DEBUGLoadBMP(gameMemory.DEBUGPlatformReadEntireFile, TEXT("test.bmp"));
		state->Background = DEBUGLoadBMP(gameMemory.DEBUGPlatformReadEntireFile, TEXT("background.bmp"));
		state->Player = DEBUGLoadBMP(gameMemory.DEBUGPlatformReadEntireFile, TEXT("player.bmp"));
	}
	
	TileMap* tileMap = world->TileMap;
	
	real32 screenCenterX = (real32) screenBuffer->Width  / 2;
	real32 screenCenterY = (real32) screenBuffer->Height / 2;
	
	const real32 playerWidth = tileMap->TileSideInMeters * .75f;
	const real32 playerHeight = (real32) tileMap->TileSideInMeters;
	
	/* Handle input */
	{
		if (input.MouseButtons[MOUSE_LEFT].EndedDown)
		{
			TileMapPosition targetTile = ScreenCoordsToTileMapPosition(*screenBuffer, tileMap, playerPosition, input.MouseX, input.MouseY);
			SetTileValue(tileMap, targetTile.AbsTileX, targetTile.AbsTileY, 1);
		}
		
		if (input.MouseButtons[MOUSE_RIGHT].EndedDown)
		{
			TileMapPosition targetTile = ScreenCoordsToTileMapPosition(*screenBuffer, tileMap, playerPosition, input.MouseX, input.MouseY);
			SetTileValue(tileMap, targetTile.AbsTileX, targetTile.AbsTileY, 0);
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
		
		_stprintf_s(LogMessage, LogMessageSize, TEXT("%f, %f\n"), playerPosition.TileRelX, playerPosition.TileRelY);
		OutputDebugString(LogMessage);
	}
	
	/* Render */
	// DrawRectangle(screenBuffer, .0f, .0f, (real32) screenBuffer->Width, (real32) screenBuffer->Height, .0f, .0f, .0f);
	DrawBMP(screenBuffer, state->Background, 0, 0);
	
	// Draw the world
	for (int relRow = -21; relRow < 21; ++relRow)
	{
		for (int relCol = -30; relCol < 30; ++relCol)
		{
			u32 col = playerPosition.AbsTileX + relCol;
			u32 row = playerPosition.AbsTileY + relRow;
			
			byte id = GetTileValue(tileMap, col, row);
			if (id > 0)
			{
				real32 gray = 0.5f;
				if (id == 1)
					gray = 1.0f;
				// Draw player tile in black
				// if ((col == playerPosition.AbsTileX) && (row == playerPosition.AbsTileY))
				// gray = 0;
				
				real32 centerX = screenCenterX - tileMap->MetersToPixels * playerPosition.TileRelX + (real32) relCol * tileMap->TileSideInPixels;
				real32 centerY = screenCenterY + tileMap->MetersToPixels * playerPosition.TileRelY - (real32) relRow * tileMap->TileSideInPixels;
				
				
				real32 minX = centerX - .5f * tileMap->TileSideInPixels;
				real32 minY = centerY - .5f * tileMap->TileSideInPixels;
				real32 maxX = centerX + .5f * tileMap->TileSideInPixels;
				real32 maxY = centerY + .5f * tileMap->TileSideInPixels;
				
				DrawRectangle(screenBuffer, minX, minY, maxX, maxY, gray, gray, gray);
			}
		}
	}
	
	// Draw the player
	
	real32 playerMinX = screenCenterX - 0.5f * tileMap->MetersToPixels * playerWidth;
	real32 playerMinY = screenCenterY - tileMap->MetersToPixels * playerHeight;
	
	// DrawRectangle(screenBuffer, playerMinX, playerMinY, playerMinX + tileMap->MetersToPixels * playerWidth, playerMinY + tileMap->MetersToPixels * playerHeight, .2f, .3f, .8f);
	DrawBMP(screenBuffer, state->Player, playerMinX, playerMinY);
	
	DrawBMP(screenBuffer, state->Test, 20.0f, 15.0f);
}


// Outputs sound for the current frame
EXPORT GAME_GET_SOUND_SAMPLES(GameGetSoundSamples /*const GameMemory& gameMemory, GameSoundOutputBuffer *soundOutput*/)
{}
