#pragma once

#include "Limit.h"

#include "String.h"

struct Win32ReplayBuffer
{
	HANDLE FileHandle;
	HANDLE MemoryMap;
	Char FileName[MAX_PATH];
	void* MemoryBlock;
};

struct Win32State
{
	void* GameMemoryBlock;
	u64 TotalMemorySize;

	Win32ReplayBuffer ReplayBuffers[4];
	HANDLE RecordingHandle;
	int InputRecordingSlot; // the index of the buffer in ReplayBuffers, -1 for nothing recording
	HANDLE PlaybackHandle;
	int InputPlayingSlot; // the index of the buffer in ReplayBuffers, -1 for nothing playing

	// The game loop uses 2 input states ("old" and "new") to handle controls. 
	GameInput Input[2] = {};
	// The input state needs to be saved each time we start and restored when we
	// stop playback to avoid a bug where if you stop playback while playing a time
	// when a button was held down, the button continues to be pressed.
	GameInput SavedInputBeforePlay[2];

	String EXEFileName, EXEDir;
};

struct Win32WindowDimension
{
	int Width, Height;
};

struct Win32OffscreenBuffer
{
	// Pixels are always 32bit wide
	BITMAPINFO Info;
	void* Memory;
	int Width, Height;
	int Pitch;
	int BytesPerPixel;
};

struct Win32GameCode
{
	HMODULE DLL;
	GameUpdateAndRenderFunc *UpdateAndRender;
	GameGetSoundSamplesFunc *GetSoundSamples;
	FILETIME LastWriteTime;
	bool IsValid;
};
