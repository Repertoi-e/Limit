#include "Platform.h"

#include <tchar.h>

#ifdef LIMIT_INTERNAL 
static DEBUGFileReadResult DEBUGPlatformReadEntireFile(const Char *filePath)
{
	DEBUGFileReadResult result = {};

	HANDLE fileHandle = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (fileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER fileSize;
		if (GetFileSizeEx(fileHandle, &fileSize))
		{
			u32 fileSize32 = SafeTruncateU64(fileSize.QuadPart);
			if (result.Contents = VirtualAlloc(0, fileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE))
			{
				DWORD bytesRead;
				if (ReadFile(fileHandle, result.Contents, fileSize32, &bytesRead, 0) && (bytesRead == fileSize32))
				{
					// Read successful!
					result.ContentsSize = bytesRead;
				}
				else
				{
					_tprintf(TEXT("Terminal failure: Unable to read from file \"%s\". Maybe the file got truncated in the middle of reading.\n GetLastError=%08x\n"), filePath, GetLastError());
					DEBUGPlatformFreeFileMemory(result.Contents);
					result.Contents = 0;
				}
			}
		}
		CloseHandle(fileHandle);
	}
	else
	{
		_tprintf(TEXT("Terminal failure: Unable to open file \"%s\" for reading.\n"), filePath);
	}

	return result;
}

static void DEBUGPlatformFreeFileMemory(void *memory)
{
	if (memory)
		VirtualFree(memory, 0, MEM_RELEASE);
}

static bool32 DEBUGPlatformWriteEntireFile(const Char *filePath, void *memory, u32 memorySize)
{
	bool32 result = 0;

	HANDLE fileHandle = CreateFile(filePath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (fileHandle != INVALID_HANDLE_VALUE)
	{
		DWORD bytesWritten;
		if (WriteFile(fileHandle, memory, memorySize, &bytesWritten, 0))
		{
			// Write (maybe) successful!
			result = bytesWritten == memorySize;
		}
		else
		{
			_tprintf(TEXT("Terminal failure: Unable to write to file \"%s\".\n GetLastError=%08x\n"), filePath, GetLastError());
		}
		CloseHandle(fileHandle);
	}
	else
	{
		_tprintf(TEXT("Terminal failure: Unable to open file \"%s\" for writing.\n"), filePath);
	}

	return result;
}
#endif
