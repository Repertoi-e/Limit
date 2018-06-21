#pragma once

#ifdef _WIN32 
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#define NOMINMAX
#undef NOGDI
#include <Windows.h>
#define NOGDI
#else
#error Unsupported platform!
#endif

#include "Types.h"

#define Assert(x) if (!(x)) { *(byte *) 0 = 0; }

#define KiloByte(x) (x * 1024LL)
#define MegaByte(x) (KiloByte(x) * 1024LL)
#define GigaByte(x) (MegaByte(x) * 1024LL)
#define TeraByte(x) (GigaByte(x) * 1024LL)

#ifdef LIMIT_INTERNAL
struct DEBUGFileReadResult
{
	void *Contents;
	u32 ContentsSize;
};
static DEBUGFileReadResult DEBUGPlatformReadEntireFile(const Char *filePath);
static void DEBUGPlatformFreeFileMemory(void *memory);
static bool32 DEBUGPlatformWriteEntireFile(const Char *filePath, void *memory, u32 memorySize);
#endif

struct GameMemory
{
	void *Permanent;
	u64 PermanentSize;
	
	void *Transient;
	u64 TransientSize;
};

struct GameState
{
	bool IsInitialized;
};

inline u32 SafeTruncateU64(u64 value)
{
	Assert(value <= 0xFFFFFFFF);
	return (u32) value;
}

static void GameUpdateAndRender(GameMemory *memory);