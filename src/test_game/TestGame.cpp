#include "Limit.h"

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

static void GameUpdateAndRender(const GameMemory& memory, const GameOffscreenBuffer& screenBuffer)
{
	GameState *state = (GameState *) memory.Permanent;
	if (!state->IsInitialized)
	{
		state->IsInitialized = true;
		/* Code that runs on game intitialization */

		auto[contents, contentsSize] = DEBUGPlatformReadEntireFile(TEXT(__FILE__));
		if (contents)
		{
			DEBUGPlatformWriteEntireFile(TEXT("test.out"), contents, contentsSize);
			DEBUGPlatformFreeFileMemory(contents);
		}
		state->ToneHz = 512;
	}
	RenderWeirdGradient(screenBuffer, 0, 0);
}

static void GameGetSoundSamples(const GameMemory& memory, const GameSoundOutputBuffer& soundOutput)
{
	GameState *state = (GameState *) memory.Permanent;

	static real32 tSine = 0.0f;
	s16 toneVolume = 3000;
	real32 wavePeriod = (real32) soundOutput.SamplesPerSecond / state->ToneHz;

	s16 *sampleOut = soundOutput.Samples;
	for (int i = 0; i < soundOutput.SampleCount; ++i)
	{
		real32 sine = sinf(tSine);
		s16 sampleValue = (s16) (sine * toneVolume);
		*sampleOut++ = sampleValue;
		*sampleOut++ = sampleValue;

		tSine += 2.f * M_PI * (1.f / wavePeriod);
		if (tSine > (2.f * M_PI))
			tSine -= (2.f * M_PI);
	}
}