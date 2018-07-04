#pragma once

#include "Types.h"

#include <cmath> // temporary

inline u32 SafeTruncateU64(u64 value)
{
	Assert(value <= 0xFFFFFFFF);
	return (u32) value;
}

inline s32 FloorToS32(real32 value)
{
	return (s32) std::floorf(value);
}

inline s32 RoundToS32(real32 value)
{
	return (s32) std::roundf(value);
}

inline u32 RoundToU32(real32 value)
{
	return (u32) (value + 0.5f);
}

inline real32 Sin(real32 value)
{
	return std::sinf(value);
}

inline real32 Cos(real32 value)
{
	return std::cosf(value);
}

inline real32 ATan2(real32 y, real32 x)
{
	return std::atan2(y, x);
}





