#pragma once

#include "Limit.h"

#include "String.h"

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

static void Win32GetEXEFileName(Win32State *state)
{
	Char *nameBuffer = state->EXEFileName.Buffer;
	GetModuleFileName(0, nameBuffer, state->EXEFileName.Length);
	Char *slash = nameBuffer;
	for (Char *scan = nameBuffer; *scan; ++scan)
		if (*scan == '\\')
			slash = scan + 1;
	state->EXEDir = String(nameBuffer, (u32) (slash - nameBuffer));
}

// Create a full path to a file that is in the same directory as the executable
// (e.g. game.dll ---> L:/bin/game.dll)
static void Win32BuildEXEPathFileName(Win32State *state, Char *fileName, String&& dest)
{
	ConcatStrings(state->EXEDir, String(fileName), (String&&) dest);
}
