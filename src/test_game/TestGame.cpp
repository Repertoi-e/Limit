#include "../Limit.h"

#include <cmath> // for std::sinf

static void RenderBackground(GameOffscreenBuffer *buffer)
{
	byte *row = (byte *) buffer->Memory;
	for (int y = 0; y < buffer->Height; ++y)
	{
		u32 *pixel = (u32 *) row;
		for (int x = 0; x < buffer->Width; ++x)
		{
			byte green = (byte) (255.f * (real32) y / buffer->Height);
			byte blue = (byte) (255.f * (real32) x / buffer->Width);
			*pixel++ = (green << 16) | blue;
		}
		row += buffer->Pitch;
	}
}

inline s32 RoundToS32(real32 value)
{
	return (s32) (value + 0.5f);
}

inline u32 RoundToU32(real32 value)
{
	return (u32) (value + 0.5f);
}

static void RenderRectangle(GameOffscreenBuffer *buffer, real32 fMinX, real32 fMinY, real32 fMaxX, real32 fMaxY, real32 r, real32 g, real32 b)
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
	if (!state->IsInitialized)
	{
		state->IsInitialized = true;

		/* Code that runs on game intitialization */
	}

	// Get references to avoid having to type state-> everywhere (syntactic sugar)
	int& playerX = state->PlayerX;
	int& playerY = state->PlayerY;

	RenderBackground(screenBuffer);

	int speed = 15;
	if (input.MoveRight.EndedDown)
		state->PlayerX += speed;
	if (input.MoveLeft.EndedDown)
		state->PlayerX -= speed;
	if (input.MoveUp.EndedDown)
		state->PlayerY -= speed;
	if (input.MoveDown.EndedDown)
		state->PlayerY += speed;

	const int playerWidth = 10, playerHeight = 10;
	RenderRectangle(screenBuffer, (real32) playerX, (real32) playerY, (real32) playerX + playerWidth, (real32) playerY + playerHeight, 1.0f, 1.0f, 0.0f);
}


// Outputs sound for the current frame
EXPORT GAME_GET_SOUND_SAMPLES(GameGetSoundSamples /*const GameMemory& gameMemory, GameSoundOutputBuffer *soundOutput*/)
{
}