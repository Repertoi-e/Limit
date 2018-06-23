#include "Limit.h"
#include "Platform.h"

#include "Win32_FileIO.cpp"

static void GameUpdateAndRender(const GameMemory& memory)
{
	GameState *state = (GameState *) memory.Permanent;
	if (!state->IsInitialized)
	{
		/* Code that runs on game intitialization */

		auto[contents, contentsSize] = DEBUGPlatformReadEntireFile(TEXT(__FILE__));
		if (contents)
		{
			DEBUGPlatformWriteEntireFile(TEXT("test.out"), contents, contentsSize);
			DEBUGPlatformFreeFileMemory(contents);
		}
		state->ToneHz = 512;

		state->IsInitialized = true;
	}
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