#include "Platform.h"

inline u32 StringLength(const Char *buffer)
{
	u32 result = 0;
	while (*buffer++)
		++result;
	return result;
}

struct String
{
	Char *Buffer = 0;
	u32 Length = 0;

	String() {}

	String(Char *buffer)
	{
		Buffer = buffer;
		Length = StringLength(Buffer);
	}

	String(Char *buffer, u32 length)
	{
		Buffer = buffer;
		Length = length;
	}
};

inline void ConcatStrings(const String& a, const String& b, String&& result)
{
	Assert(result.Length > a.Length + b.Length);

	Char *dest = result.Buffer;
	const Char *aBuffer = a.Buffer, *bBuffer = b.Buffer;
	for (size_t i = 0; i < a.Length; ++i)
		*dest++ = *aBuffer++;
	for (size_t i = 0; i < b.Length; ++i)
		*dest++ = *bBuffer++;
	*dest++ = 0;
}

