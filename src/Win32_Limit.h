#pragma once

#include "Limit.h"

struct Win32State
{
	Char EXEFileName[MAX_PATH];
	Char *EXEFileNameSlash;
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
