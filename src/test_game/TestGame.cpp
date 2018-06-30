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

inline int RoundToS32(real32 value)
{
	return (s32) (value + 0.5f);
}

static void RenderRectangle(GameOffscreenBuffer *buffer, real32 fMinX, real32 fMinY, real32 fMaxX, real32 fMaxY, u32 color)
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
		state->ToneHz = 512;
		state->tSine = 0.0f;
	}

	// Get references to avoid having to type state-> everywhere (syntactic sugar)
	int& playerX = state->PlayerX;
	int& playerY = state->PlayerY;
	int& playerVelocityY = state->PlayerVelocityY;
	bool& playerJumped = state->PlayerJumped;

	RenderBackground(screenBuffer);

	const int playerWidth = 10, playerHeight = 10;
#if 0
	// This is a simple example, which draws a square (the "player") at the cursor's position
	playerX = input.MouseX;
	playerY = input.MouseY;
#else
	// This example is a more platform-like player movement
	int speed = 15; // (int) ((real32) input.MouseX / screenBuffer->Width * 50);
	if (input.MoveRight.EndedDown)
		state->PlayerX += speed;
	if (input.MoveLeft.EndedDown)
		state->PlayerX -= speed;
	if (input.Jump.EndedDown)
	{
		if (!playerJumped)
		{
			playerVelocityY -= 40;
			playerJumped = true;
		}
	}
	playerY += playerVelocityY;
	
	// Down is positive Y 
	playerVelocityY += 9;

	const int floorHeight = screenBuffer->Height - 400;


	if (playerY > floorHeight - playerHeight)
	{
		playerVelocityY = 0;
		playerY = floorHeight - playerHeight;
		playerJumped = false;
	}
	RenderRectangle(screenBuffer, 0, floorHeight, screenBuffer->Width, 500, 0xFF00FFFF);

#endif

	RenderRectangle(screenBuffer, (real32) playerX, (real32) playerY, (real32) playerX + playerWidth, (real32) playerY + playerHeight, 0xFFFF00FF);
}


// Outputs sound for the current frame,
// just a simple sine wave for now.
EXPORT GAME_GET_SOUND_SAMPLES(GameGetSoundSamples /*const GameMemory& gameMemory, GameSoundOutputBuffer *soundOutput*/)
{
	GameState *state = (GameState *) gameMemory.Permanent;

	s16 toneVolume = 3000;
	real32 wavePeriod = (real32) soundOutput->SamplesPerSecond / state->ToneHz;

	s16 *sampleOut = soundOutput->Samples;
	for (int i = 0; i < soundOutput->SampleCount; ++i)
	{
		real32 sine = std::sinf(state->tSine);
		s16 sampleValue =
	#if 0
			(s16) (sine * toneVolume);
	#else
			0;
	#endif
		*sampleOut++ = sampleValue;
		*sampleOut++ = sampleValue;

		state->tSine += 2.f * M_PI * (1.f / wavePeriod);
		if (state->tSine > (2.f * M_PI))
			state->tSine -= (2.f * M_PI);
	}
}