#include "../Limit.h"

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

static void RenderPlayer(GameOffscreenBuffer *buffer, int playerX, int playerY)
{
	const u32 playerColor = 0xFF00FF00;
	const int playerWidth = 10, playerHeight = 10;

	byte *bufferEnd = ((byte *) buffer->Memory) + (buffer->Pitch * buffer->Height);
	for (int x = playerX; x < playerX + playerWidth; ++x)
	{
		byte *pixel = (((byte *) buffer->Memory) + x * buffer->BytesPerPixel + playerY * buffer->Pitch);

		for (int y = playerY; y < playerY + playerHeight; ++y)
		{
			if (pixel >= buffer->Memory && ((pixel + 4) < bufferEnd))
			{
				*(u32*) pixel = playerColor;
				pixel += buffer->Pitch;
			}
		}
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

	// This is a simple example, which draws a square (the "player") at the cursor's position
	RenderBackground(screenBuffer);

#if 1
	int speed = (int) ((real32) input.MouseX / screenBuffer->Width * 50);
	if (input.MoveUp.EndedDown)
		state->PlayerY -= speed;
	if (input.MoveDown.EndedDown)
		state->PlayerY += speed;
	if (input.MoveLeft.EndedDown)
		state->PlayerX -= speed;
	if (input.MoveRight.EndedDown)
		state->PlayerX += speed;
#else
	state->PlayerX = input.MouseX;
	state->PlayerY = input.MouseY;
#endif

	RenderPlayer(screenBuffer, state->PlayerX, state->PlayerY);
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
		real32 sine = sinf(state->tSine);
		s16 sampleValue =
		#if 1
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