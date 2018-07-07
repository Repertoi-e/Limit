#pragma once

#if !defined COMPILER_MSVC
#define COMPILER_MSVC 0
#endif

#if !COMPILER_MSVC
#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#endif
#else
// More compilers!
#endif

#ifdef _WIN32 
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#define NOMINMAX
#undef NOGDI
#include <Windows.h>
#include <tchar.h>
#define NOGDI
#define EXPORT __declspec(dllexport)
#else
#error Unsupported platform!
#endif

#include "Types.h"

#define Assert(x) if (!(x)) { *(byte *) 0 = 0; }
#define ArrayCount(x) (sizeof(x) / sizeof(x[0]))

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

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) DEBUGFileReadResult name(const Char *fileName)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFileFunc);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(const Char *fileName, void *memory, u32 memorySize)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFileFunc);
#endif

// Memory arena
struct MemoryArena
{
	byte *Base;
	u64 Used;
	u64 Size;
};

#define PushStruct(arena, type) (type *) PushSize_(arena, sizeof(type))
#define PushArray(arena, count, type) (type *) PushSize_(arena, (count) * sizeof(type))

static byte* PushSize_(MemoryArena *arena, u32 size)
{
	Assert(arena->Used + size <= arena->Size);
	byte *result = arena->Base + arena->Used;
	arena->Used += size;
	return result;
}
