#pragma once

#include "Types.h"

struct GameMemory
{
	void *Permanent;
	u64 PermanentSize;

	void *Transient;
	u64 TransientSize;
};

struct GameState
{
	int ToneHz;
	bool IsInitialized;
};

struct GameSoundOutputBuffer
{
	int SamplesPerSecond;
	int SampleCount;
	s16 *Samples;
};

static void GameUpdateAndRender(const GameMemory& memory);
// At the moment, this has to be a very fast function it cannot be more than a millisecond or so.
static void GameGetSoundSamples(const GameMemory& memory, const GameSoundOutputBuffer& soundOutput);
