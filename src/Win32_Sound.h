#pragma once

#include "Types.h"

struct Win32SoundOutput
{
	int SamplesPerSecond;
	u32 RunningSampleIndex;
	int BytesPerSample;
	int SecondaryBufferSize;
	u32 SafetyBytes;
};