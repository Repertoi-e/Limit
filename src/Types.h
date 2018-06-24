#pragma once

#include <stdint.h>

using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using real32 = float;
using real64 = double;

using byte = u8;

using wchar = wchar_t;
using wbyte = wchar;

// Stores either: 0 - false, 
//	or anything else - true
using bool32 = int;

// String unicode typedef
#include <string>

#if defined UNICODE || defined _UNICODE
using String = std::wstring;
using Char = wchar;
#else
using String = std::string;
using Char = char;
#endif
