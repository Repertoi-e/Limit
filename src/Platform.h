#pragma once

#ifdef _WIN32 
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#define NOMINMAX
#undef NOGDI
#include <Windows.h>
#include <tchar.h>
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

#define M_PI 3.14159265358979323846f

// Debug File I/O
#ifdef LIMIT_INTERNAL
struct DEBUGFileReadResult
{
	void *Contents;
	u32 ContentsSize;
};

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(void *memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemoryFunc);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) DEBUGFileReadResult name(const Char *filePath)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFileFunc);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(const Char *filePath, void *memory, u32 memorySize)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFileFunc);
#endif

inline u32 SafeTruncateU64(u64 value)
{
	Assert(value <= 0xFFFFFFFF);
	return (u32) value;
}
