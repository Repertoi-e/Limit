#pragma once

#include "Platform.h"

/* Game Services */

struct GameMemory
{
	void *Permanent;
	u64 PermanentSize;

	void *Transient;
	u64 TransientSize;

	DEBUGPlatformFreeFileMemoryFunc *DEBUGPlatformFreeFileMemory;
	DEBUGPlatformReadEntireFileFunc *DEBUGPlatformReadEntireFile;
	DEBUGPlatformWriteEntireFileFunc *DEBUGPlatformWriteEntireFile;
};

struct GameState
{
	int ToneHz;
	bool IsInitialized;
	real32 tSine;
};

struct GameOffscreenBuffer
{
	// Pixels are always 32bit wide
	void* Memory;
	int Width, Height;
	int Pitch;
};

struct GameSoundOutputBuffer
{
	int SamplesPerSecond;
	int SampleCount;
	s16 *Samples;
};

/* arg1: const GameMemory& gameMemory, arg2: const GameOffscreenBuffer& screenBuffer */
#define GAME_UPDATE_AND_RENDER(name) void name(const GameMemory& gameMemory, const GameOffscreenBuffer& screenBuffer)
typedef GAME_UPDATE_AND_RENDER(GameUpdateAndRenderFunc);
GAME_UPDATE_AND_RENDER(GameUpdateAndRenderStub)
{}

// At the moment, this has to be a very fast function it cannot be more than a millisecond or so.
/* arg1: const GameMemory& gameMemory, arg2: const GameSoundOutputBuffer& soundOutput */
#define GAME_GET_SOUND_SAMPLES(name) void name(const GameMemory& gameMemory, const GameSoundOutputBuffer& soundOutput)
typedef GAME_GET_SOUND_SAMPLES(GameGetSoundSamplesFunc);
GAME_GET_SOUND_SAMPLES(GameGetSoundSamplesStub)
{}
