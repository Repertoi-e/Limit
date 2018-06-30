#pragma once

#include "Platform.h"

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
	real32 tSine;

	int PlayerX, PlayerY;
	int PlayerVelocityY;
	bool PlayerJumped;

	bool IsInitialized;
};

struct GameOffscreenBuffer
{
	// Pixels are always 32bit wide
	void* Memory;
	int Width, Height;
	int Pitch;
	int BytesPerPixel;
};

struct GameSoundOutputBuffer
{
	int SamplesPerSecond;
	int SampleCount;
	s16 *Samples;
};

struct GameButtonState
{
	int HalfTransitionCount;
	bool32 EndedDown;
};

struct GameInput
{
	int MouseX, MouseY;
	GameButtonState MouseButtons[5]; // In order: LB, MB, RB, X1, X2

	union
	{
		GameButtonState Buttons[5];
		struct
		{
			GameButtonState MoveUp;
			GameButtonState MoveDown;
			GameButtonState MoveLeft;
			GameButtonState MoveRight;
			GameButtonState Jump;
		};
	};
};

/* Game services */
#define GAME_UPDATE_AND_RENDER(name) void name(const GameMemory& gameMemory, const GameInput& input, GameOffscreenBuffer* screenBuffer)
typedef GAME_UPDATE_AND_RENDER(GameUpdateAndRenderFunc);

// At the moment, this has to be a very fast function. It cannot be more than a millisecond or so.
#define GAME_GET_SOUND_SAMPLES(name) void name(const GameMemory& gameMemory, GameSoundOutputBuffer *soundOutput)
typedef GAME_GET_SOUND_SAMPLES(GameGetSoundSamplesFunc);
