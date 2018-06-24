#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")

// Every game provides "services" for the platform
// (e.g. GameUpdateAndRender) (Look in Platform.h)
// instead of abstracting window handles and methods, etc.
// (Does the game really need to have any
// idea what a window is or how to manage it?)
//
// The idea is to make it very easy for developers
// to port to another platform. Basically the platform
// layer is supposed to ask the game to do stuff instead
// of the other way around, which makes code easy to follow
// and straightforward. The only instance when game
// code calls stuff from the platform layer is for file I/O.

#include "Win32_Limit.h"
#include "Win32_FileIO.cpp"
#include "Win32_Sound.cpp"

#include <TimeAPI.h>

static Win32OffscreenBuffer g_BackBuffer;

static Win32WindowDimension Win32GetWindowDimension(HWND window)
{
	RECT rect;
	GetClientRect(window, &rect);
	return {rect.right - rect.left, rect.bottom - rect.top};
}

static void Win32ResizeDIBSection(Win32OffscreenBuffer& buffer, int width, int height)
{
	if (buffer.Memory)
		VirtualFree(buffer.Memory, 0, MEM_RELEASE);

	buffer.Width = width;
	buffer.Height = height;
	buffer.BytesPerPixel = 4;
	{
		buffer.Info.bmiHeader.biSize = sizeof(buffer.Info.bmiHeader);
		buffer.Info.bmiHeader.biWidth = buffer.Width;
		buffer.Info.bmiHeader.biHeight = -buffer.Height; //negative is top-down, positive is bottom-up
		buffer.Info.bmiHeader.biPlanes = 1;
		buffer.Info.bmiHeader.biBitCount = 32;
		buffer.Info.bmiHeader.biCompression = BI_RGB;
	}

	buffer.Memory = VirtualAlloc(0, buffer.Width * buffer.Height * buffer.BytesPerPixel, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	buffer.Pitch = width * buffer.BytesPerPixel;

	// #TODO Probably clear this to black
}

static void Win32DisplayBufferInWindow(const Win32OffscreenBuffer& buffer, HDC hdc, int width, int height)
{
	//TODO: aspect ratio coeection
	//TODO: Play with strech modes
	StretchDIBits(hdc,
				  0, 0, width, height,				 //X, Y, Width, Height,
				  0, 0, buffer.Width, buffer.Height, //X, Y, Width, Height,
				  buffer.Memory,
				  &buffer.Info,
				  DIB_RGB_COLORS,
				  SRCCOPY);
}

inline FILETIME Win32GetLastWriteTime(const Char *filename)
{
	FILETIME result = {};
	
	WIN32_FILE_ATTRIBUTE_DATA fileData;
	if (GetFileAttributesEx(filename, GetFileExInfoStandard, &fileData))
		result = fileData.ftLastWriteTime;
	return result;
}

static Win32GameCode Win32LoadGameCode(const Char *sourceDLLName, const Char *tempDLLName)
{
	Win32GameCode result = {};

	result.LastWriteTime = Win32GetLastWriteTime(sourceDLLName);

	// Wait for the compiler to unlock the .dll so we can copy it,
	// so we try CreateFile until it succeeds.
	// #TODO Maybe find a way to implement this properly...
	HANDLE dummy;
	while ((dummy = CreateFile(sourceDLLName, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0)) == INVALID_HANDLE_VALUE)
		Sleep(5);
	CloseHandle(dummy);
	
	CopyFile(sourceDLLName, tempDLLName, FALSE);

	result.DLL = LoadLibrary(tempDLLName);
	if (result.DLL)
	{
		result.UpdateAndRender = (GameUpdateAndRenderFunc *)GetProcAddress(result.DLL, "GameUpdateAndRender");
		result.GetSoundSamples = (GameGetSoundSamplesFunc *)GetProcAddress(result.DLL, "GameGetSoundSamples");
		result.IsValid = result.UpdateAndRender && result.GetSoundSamples;
	}

	if (!result.IsValid)
	{
		result.UpdateAndRender = 0;
		result.GetSoundSamples = 0;
	}
	return result;
}

static void Win32UnloadGameCode(Win32GameCode *gameCode)
{
	if (gameCode->DLL)
	{
		FreeLibrary(gameCode->DLL);
		gameCode->DLL = 0;
	}

	gameCode->IsValid = false;
	gameCode->UpdateAndRender = 0;
	gameCode->GetSoundSamples = 0;
}

static LRESULT CALLBACK WindowProc(HWND window, u32 message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
	{
		PostQuitMessage(0);
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT paint;
		HDC hdc = BeginPaint(window, &paint);

		Win32WindowDimension dimension = Win32GetWindowDimension(window);
		Win32DisplayBufferInWindow(g_BackBuffer, hdc, dimension.Width, dimension.Height);

		EndPaint(window, &paint);
	}
		return 0;
	}
	return DefWindowProc(window, message, wParam, lParam);
}

static s64 Win32GetPerformanceFrequency()
{
	LARGE_INTEGER query;
	QueryPerformanceFrequency(&query);
	return query.QuadPart;
}

static const s64 g_PerformanceFrequency = Win32GetPerformanceFrequency();

inline LARGE_INTEGER Win32GetWallClock()
{
	LARGE_INTEGER result;
	QueryPerformanceCounter(&result);
	return result;
}

inline real32 Win32GetSecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end)
{
	return (real32)(end.QuadPart - start.QuadPart) / (real32)g_PerformanceFrequency;
}

static size_t StringLength(Char *string)
{ 
	size_t len = 0;
    while (*string++) 
        ++len; 
    return len; 
} 

static void CatStrings(Char *a, size_t aLen, Char *b, size_t bLen, Char *dest, size_t destLen)
{
	Assert(aLen + bLen < destLen) 
	for (size_t i = 0; i < aLen; ++i)
		*dest++ = *a++;
	for (size_t i = 0; i < bLen; ++i)
		*dest++ = *b++;
	*dest++ = 0;
}

static void Win32GetEXEFileName(Win32State *state)
{
    GetModuleFileName(0, state->EXEFileName, sizeof(state->EXEFileName));
    state->EXEFileNameSlash = state->EXEFileName;
    for (Char *scan = state->EXEFileName; *scan; ++scan)
        if (*scan == '\\')
            state->EXEFileNameSlash = scan + 1;
}

static void Win32BuildEXEPathFileName(Win32State *state, Char *fileName, Char *dest, size_t destLen)
{
    CatStrings(state->EXEFileName, state->EXEFileNameSlash - state->EXEFileName, fileName, StringLength(fileName), dest, destLen);
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE, PSTR, int)
{
	constexpr size_t LogMessageSize = 512;
	Char LogMessage[LogMessageSize]; // Buffer used for _stprintf_s-ing text in VS Debug output

	Win32State state;
	Win32GetEXEFileName(&state);

	// Setup .dll locations
	// Compile game code separately from engine
	// to allow code recompilation on the fly.
	// Change literals below to match game code .dll.
    Char sourceGameCodeDLLFullPath[MAX_PATH];
    Win32BuildEXEPathFileName(&state, TEXT("TestGame.dll"),
        sourceGameCodeDLLFullPath, sizeof(sourceGameCodeDLLFullPath) / sizeof(Char));

    Char tempGameCodeDLLFullPath[MAX_PATH];
    Win32BuildEXEPathFileName(&state, TEXT("TestGame.temp.dll"),
        tempGameCodeDLLFullPath, sizeof(tempGameCodeDLLFullPath) / sizeof(Char));

	WNDCLASS wndClass;
	ZeroMemory(&wndClass, sizeof(WNDCLASS));
	{
		wndClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
		wndClass.lpfnWndProc = WindowProc;
		wndClass.hInstance = instance;
		wndClass.hIcon = LoadIcon(NULL, IDI_WINLOGO);
		wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndClass.lpszClassName = TEXT("LimitClassName");
	}
	if (RegisterClass(&wndClass))
	{
		int width = 800, height = 600;
		const Char *windowTitle = TEXT("Limit Test");

		DWORD exStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

		RECT size = {0, 0, width, height};
		AdjustWindowRectEx(&size, style, false, exStyle);
		Win32ResizeDIBSection(g_BackBuffer, width, height);

		HWND window = CreateWindowEx(exStyle,
									 wndClass.lpszClassName,
									 windowTitle,
									 style,
									 CW_USEDEFAULT, CW_USEDEFAULT,
									 size.right + (-size.left), size.bottom + (-size.top),
									 NULL, NULL, instance, NULL);
		if (window)
		{
			SetFocus(window);

			// Allocate the memory that the game uses during
			// execution. Allocating a big chunk at the beginning
			// of execution makes sure that it has enough memory
			// to run. While this might be overkill for PC, where
			// you rarely run out of memory during execution, on
			// systems with fixed memory (like consoles) this makes
			// sure that there are no unexpected crashes when
			// dynamically allocating memory all the time.
			GameMemory gameMemory;
			gameMemory.PermanentSize = KiloByte(128);
			gameMemory.TransientSize = KiloByte(256);

			u64 totalSize = gameMemory.PermanentSize + gameMemory.TransientSize;
			gameMemory.Permanent = VirtualAlloc(0, (size_t)totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			gameMemory.Transient = ((byte *)gameMemory.Permanent + gameMemory.PermanentSize);

			gameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
			gameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
			gameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;

			if (gameMemory.Permanent && gameMemory.Transient)
			{
				// #Hardcoded
#define DISPLAY_HZ (60)
#define GAME_UPDATE_HZ (DISPLAY_HZ / 2)
				const real32 targetSecondsPerFrame = 1.f / (real32)GAME_UPDATE_HZ;

				// #define SOUND_DEBUG_INFO

				const u32 desiredSchedulerMS = 1;
				const bool hasGranularSleep = timeBeginPeriod(desiredSchedulerMS) == TIMERR_NOERROR;

				Win32SoundOutput soundOutput;
				ZeroMemory(&soundOutput, sizeof(Win32SoundOutput));
				{
					soundOutput.SamplesPerSecond = 48000;
					soundOutput.BytesPerSample = sizeof(s16) * 2;
					soundOutput.SecondaryBufferSize = soundOutput.SamplesPerSecond * soundOutput.BytesPerSample;
					soundOutput.SafetyBytes = (soundOutput.SamplesPerSecond * soundOutput.BytesPerSample / GAME_UPDATE_HZ) / 3;
				}
				Win32InitDSound(window, soundOutput.SamplesPerSecond, soundOutput.SecondaryBufferSize);
				Win32ClearSoundBuffer(&soundOutput);
				g_SecondarySoundBuffer->Play(0, 0, DSBPLAY_LOOPING);

				s16 *soundSamples = (s16 *)VirtualAlloc(0, soundOutput.SecondaryBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#if defined SOUND_DEBUG_INFO
				u32 audioLatencyBytes;
				real32 audioLatencySeconds;
#endif
				bool32 validSound = false;

				HDC hdc = GetDC(window);

				LARGE_INTEGER lastCounter = Win32GetWallClock();
				LARGE_INTEGER flipWallClock = lastCounter;

				// Load the game code for the first time
				Win32GameCode game = Win32LoadGameCode(sourceGameCodeDLLFullPath, tempGameCodeDLLFullPath);

				bool running = true;
				while (running)
				{
					// Check for game code .dll change by comparing the dates, if it is - reload it
					FILETIME newWriteTime = Win32GetLastWriteTime(sourceGameCodeDLLFullPath);

					if (CompareFileTime(&newWriteTime, &game.LastWriteTime) != 0)
					{
						Win32UnloadGameCode(&game);
						game = Win32LoadGameCode(sourceGameCodeDLLFullPath, tempGameCodeDLLFullPath);
					}

					MSG msg;
					while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
					{
						if (msg.message == WM_QUIT)
							running = false;
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}

					GameOffscreenBuffer screenBuffer;
					ZeroMemory(&screenBuffer, sizeof(GameOffscreenBuffer));
					{
						screenBuffer.Memory = g_BackBuffer.Memory;
						screenBuffer.Width = g_BackBuffer.Width;
						screenBuffer.Height = g_BackBuffer.Height;
						screenBuffer.Pitch = g_BackBuffer.Pitch;
					}
					if (game.UpdateAndRender)
						game.UpdateAndRender(gameMemory, screenBuffer);

					/* Sound */
					LARGE_INTEGER audioWallClock = Win32GetWallClock();
					real32 elapsedSecondsToAudio = Win32GetSecondsElapsed(flipWallClock, audioWallClock);

					DWORD playCursor;
					DWORD writeCursor;
					if (g_SecondarySoundBuffer->GetCurrentPosition(&playCursor, &writeCursor) == DS_OK)
					{
						/* How sound output computaion works.

						We define a safety value that is the number of samples
						we think out game update loop may vary by (let's say
						up to 2ms).

						When we wake up to write audio, we will look and see what the play
						cursor position is and we will forecast ahead where we whink the
						play cursor will be on the next frame boundary.

						We will then look to see if the write cursor is before that by at
						least our safety value. If it is, the target fill position is that
						frame boundary plus one frame. This gives us perfect audio sync in the
						case of a card that has low enough latency.

						If the write cursor is _after_ that safety margin, then we assume
						we can never sync the audio perfectly, so we will write one frame's
						worth of audio plus the safety margin's worth of guard samples.
						*/
						if (!validSound)
						{
							soundOutput.RunningSampleIndex = writeCursor / soundOutput.BytesPerSample;
							validSound = true;
						}
						u32 byteToLock = ((soundOutput.RunningSampleIndex * soundOutput.BytesPerSample) % soundOutput.SecondaryBufferSize);
						u32 expectedBytesPerFrame = (soundOutput.SamplesPerSecond * soundOutput.BytesPerSample) / GAME_UPDATE_HZ;

						real32 secondsUntilFlip = targetSecondsPerFrame - elapsedSecondsToAudio;
						u32 expectedBytesUntilFlip = (DWORD)((secondsUntilFlip / targetSecondsPerFrame) * (real32)expectedBytesPerFrame);
						u32 expectedFrameBoundaryByte = playCursor + expectedBytesUntilFlip;

						u32 safeWriteCursor = writeCursor;
						if (safeWriteCursor < playCursor)
							safeWriteCursor += soundOutput.SecondaryBufferSize;
						Assert(safeWriteCursor >= playCursor);
						safeWriteCursor += soundOutput.SafetyBytes;

						bool32 isLowLatency = (safeWriteCursor < expectedFrameBoundaryByte);

						u32 targetCursor = 0;
						if (isLowLatency)
							targetCursor = (expectedFrameBoundaryByte + expectedBytesPerFrame);
						else
							targetCursor = (writeCursor + expectedBytesPerFrame + soundOutput.SafetyBytes);
						targetCursor %= soundOutput.SecondaryBufferSize;

						u32 bytesToWrite = 0;
						if (byteToLock > targetCursor)
						{
							bytesToWrite = soundOutput.SecondaryBufferSize - byteToLock;
							bytesToWrite += targetCursor;
						}
						else
						{
							bytesToWrite = targetCursor - byteToLock;
						}
						GameSoundOutputBuffer soundBuffer;
						ZeroMemory(&soundBuffer, sizeof(GameSoundOutputBuffer));
						{
							soundBuffer.SamplesPerSecond = soundOutput.SamplesPerSecond;
							soundBuffer.SampleCount = bytesToWrite / soundOutput.BytesPerSample;
							soundBuffer.Samples = soundSamples;
						}
						if (game.GetSoundSamples)
							game.GetSoundSamples(gameMemory, soundBuffer);

#if defined SOUND_DEBUG_INFO
						u32 UnwrappedWriteCursor = writeCursor;
						if (UnwrappedWriteCursor < playCursor)
							UnwrappedWriteCursor += soundOutput.SecondaryBufferSize;
						audioLatencyBytes = (UnwrappedWriteCursor - playCursor);
						audioLatencySeconds = (((real32)audioLatencyBytes / (real32)soundOutput.BytesPerSample) / (real32)soundOutput.SamplesPerSecond);

						// samplesPerSecond already account for the two channels
						_stprintf_s(LogMessage, LogMessageSize, TEXT("BTL:%u TC:%u BTW:%u PC:%u WC:%u DELTA:%u (%fs)FlipWallClockn"), byteToLock, targetCursor, bytesToWrite, playCursor, writeCursor, audioLatencyBytes, audioLatencySeconds);
						OutputDebugString(LogMessage);
#endif
						Win32FillSoundBuffer(soundOutput, byteToLock, bytesToWrite, soundBuffer);
					}
					else
					{
						validSound = false;
					}

					// End frame
					LARGE_INTEGER workCounter = Win32GetWallClock();
					real32 workSecondsElapsed = Win32GetSecondsElapsed(lastCounter, workCounter);

					real32 frameSecondsElapsed = workSecondsElapsed;

					if (frameSecondsElapsed < targetSecondsPerFrame)
					{
						if (hasGranularSleep)
						{
							u32 sleepMS = (u32)(1000.f * (targetSecondsPerFrame - frameSecondsElapsed));
							if (sleepMS > 1)
								Sleep(sleepMS - 1);
						}
						while (frameSecondsElapsed < targetSecondsPerFrame)
							frameSecondsElapsed = Win32GetSecondsElapsed(lastCounter, Win32GetWallClock());
					}
					else
					{
						// #Logging Missed a frame!
					}

					LARGE_INTEGER endCounter = Win32GetWallClock();
					real32 msPerFrame = 1000.f * Win32GetSecondsElapsed(lastCounter, endCounter);
					lastCounter = endCounter;

					// Flip here
					Win32WindowDimension dimension = Win32GetWindowDimension(window);
					Win32DisplayBufferInWindow(g_BackBuffer, hdc, dimension.Width, dimension.Height);

					flipWallClock = Win32GetWallClock();

					// Display stats in window title
					real64 fps = 1000.0f / msPerFrame;
					_stprintf_s(LogMessage, LogMessageSize, TEXT("%s | %.02fms/f, %.02ff/s, (%.02fws/f)\n"), windowTitle, msPerFrame, fps, workSecondsElapsed * 1000.0f);
					SetWindowText(window, LogMessage);
				}
			}
		}
		else
		{
			_stprintf_s(LogMessage, LogMessageSize, TEXT("Terminal failure: Unable to create a window.\n GetLastError=%08x\n"), GetLastError());
			OutputDebugString(LogMessage);
			return -1;
		}
	}
	else
	{
		_stprintf_s(LogMessage, LogMessageSize, TEXT("Terminal failure: Unable to register internal window class.\n GetLastError=%08x\n"), GetLastError());
		OutputDebugString(LogMessage);
		return -1;
	}

	return 0;
}