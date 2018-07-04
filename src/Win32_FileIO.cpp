#ifdef LIMIT_INTERNAL 

#include "Platform.h"
#include "Intrinsics.h"

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
	if (memory)
		VirtualFree(memory, 0, MEM_RELEASE);
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
	DEBUGFileReadResult result = {};
	
	HANDLE fileHandle = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
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
					Char buffer[125 + MAX_PATH];
					_stprintf_s(buffer, 125 + MAX_PATH, TEXT("Terminal failure: Unable to read from file \"%ls\". Maybe the file got truncated in the middle of reading.\n GetLastError=%08x\n"), fileName, GetLastError());
					OutputDebugString(buffer);
					DEBUGPlatformFreeFileMemory(result.Contents);
					result.Contents = 0;
				}
			}
		}
		CloseHandle(fileHandle);
	}
	else
	{
		Char buffer[57 + MAX_PATH];
		_stprintf_s(buffer, 57 + MAX_PATH, TEXT("Terminal failure: Unable to open file \"%s\" for reading.\n"), fileName);
		OutputDebugString(buffer);
	}
	
	return result;
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
{
	bool32 result = 0;
	
	HANDLE fileHandle = CreateFile(fileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
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
			Char buffer[69 + MAX_PATH];
			_stprintf_s(buffer, 69 + MAX_PATH, TEXT("Terminal failure: Unable to write to file \"%ls\".\n GetLastError=%08x\n"), fileName, GetLastError());
			OutputDebugString(buffer);
		}
		CloseHandle(fileHandle);
	}
	else
	{
		Char buffer[57 + MAX_PATH];
		_stprintf_s(buffer, 57 + MAX_PATH, TEXT("Terminal failure: Unable to open file \"%s\" for writing.\n"), fileName);
		OutputDebugString(buffer);
	}
	
	return result;
}
#endif
