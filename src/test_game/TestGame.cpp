#include "../Limit.h"

static void RenderWeirdGradient(const GameOffscreenBuffer& buffer, int xOffset, int yOffset)
{
	byte *row = (byte *) buffer.Memory;
	for (int y = 0; y < buffer.Height; ++y)
	{
		u32 *pixel = (u32 *) row;
		for (int x = 0; x < buffer.Width; ++x)
		{
			byte green = (byte) (y + yOffset);
			byte blue = (byte) (x + xOffset);
			/*
			Memory:   BB GG RR xx
			Register: xx RR GG BB
			Pixel (32-bits)
			*/
			*pixel++ = (green << 8) | blue;
		}
		row += buffer.Pitch;
	}
}

EXPORT GAME_UPDATE_AND_RENDER(GameUpdateAndRender /*const GameMemory& gameMemory, const GameOffscreenBuffer& screenBuffer*/)
{
	GameState *state = (GameState *) gameMemory.Permanent;
	if (!state->IsInitialized)
	{
		state->IsInitialized = true;
		/* Code that runs on game intitialization */

	#if 0
		// Test debug file I/O
		auto[contents, contentsSize] = gameMemory.DEBUGPlatformReadEntireFile(TEXT(__FILE__));
		if (contents)
		{
			gameMemory.DEBUGPlatformWriteEntireFile(TEXT("test.out"), contents, contentsSize);
			gameMemory.DEBUGPlatformFreeFileMemory(contents);
		}
	#endif
		state->ToneHz = 512;
		state->tSine = 0.0f;
	}
	RenderWeirdGradient(screenBuffer, 0, 0);
}

EXPORT GAME_GET_SOUND_SAMPLES(GameGetSoundSamples /* const GameMemory& memory, const GameSoundOutputBuffer& soundOutput */)
{
	GameState *state = (GameState *) gameMemory.Permanent;

	s16 toneVolume = 3000;
	real32 wavePeriod = (real32) soundOutput.SamplesPerSecond / state->ToneHz;

	s16 *sampleOut = soundOutput.Samples;
	for (int i = 0; i < soundOutput.SampleCount; ++i)
	{
		real32 sine = sinf(state->tSine);
		s16 sampleValue = (s16) (sine * toneVolume);
		*sampleOut++ = sampleValue;
		*sampleOut++ = sampleValue;

		state->tSine += 2.f * M_PI * (1.f / wavePeriod);
		if (state->tSine > (2.f * M_PI))
			state->tSine -= (2.f * M_PI);
	}
}