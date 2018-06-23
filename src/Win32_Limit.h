#pragma once

#include "Platform.h"

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
