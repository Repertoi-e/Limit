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
#include "Win32_Timer.cpp"

#include <TimeAPI.h>

static Win32OffscreenBuffer g_BackBuffer;

static Win32WindowDimension Win32GetWindowDimension(HWND window)
{
	RECT rect;
	GetClientRect(window, &rect);
	return { rect.right - rect.left, rect.bottom - rect.top };
}

// !!!
// TEMPORARY RENDERING, REPLACE WITH A PROPER RENDERING API (DIRECT3D/OPENGL) LATER
// !!!
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

static void Win32DisplayBufferInWindow(const Win32OffscreenBuffer& buffer, HDC deviceContext, int width, int height)
{
	// #TODO Aspect ratio correction
	StretchDIBits(deviceContext,
		0, 0, buffer.Width, buffer.Height, //X, Y, Width, Height,
		0, 0, buffer.Width, buffer.Height, //X, Y, Width, Height,
		buffer.Memory,
		&buffer.Info,
		DIB_RGB_COLORS,
		SRCCOPY);
}

inline FILETIME Win32GetLastWriteTime(const Char *fileName)
{
	FILETIME result = {};

	WIN32_FILE_ATTRIBUTE_DATA fileData;
	if (GetFileAttributesEx(fileName, GetFileExInfoStandard, &fileData))
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
		result.UpdateAndRender = (GameUpdateAndRenderFunc *) GetProcAddress(result.DLL, "GameUpdateAndRender");
		result.GetSoundSamples = (GameGetSoundSamplesFunc *) GetProcAddress(result.DLL, "GameGetSoundSamples");
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

static void Win32ProcessKeyboardMessage(GameButtonState *newState, bool32 isDown)
{
	if (newState->EndedDown != isDown)
	{
		newState->EndedDown = isDown;
		++(newState->HalfTransitionCount);
	}
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
		HDC deviceContext = BeginPaint(window, &paint);

		Win32WindowDimension dimension = Win32GetWindowDimension(window);
		Win32DisplayBufferInWindow(g_BackBuffer, deviceContext, dimension.Width, dimension.Height);

		EndPaint(window, &paint);
	}
	return 0;
	}
	return DefWindowProc(window, message, wParam, lParam);
}

static size_t StringLength(Char *string)
{
	size_t len = 0;
	while (*string++)
		++len;
	return len;
}

static void ConcatStrings(Char *a, size_t aLen, Char *b, size_t bLen, Char *dest, size_t destLen)
{
	Assert(aLen + bLen < destLen);
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

// Create a full path to a file that is in the same directory as the executable
// (e.g. game.dll ---> L:/bin/game.dll)
static void Win32BuildEXEPathFileName(Win32State *state, Char *fileName, Char *dest, size_t destLen)
{
	ConcatStrings(state->EXEFileName, state->EXEFileNameSlash - state->EXEFileName, fileName, StringLength(fileName), dest, destLen);
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE, PSTR, int)
{
	constexpr size_t LogMessageSize = 512;
	Char LogMessage[LogMessageSize]; // Buffer used for _stprintf_s-ing text in VS Debug output

	Win32State win32State;
	Win32GetEXEFileName(&win32State);

	// Setup .dll locations
	// Compile game code separately from engine
	// to allow code recompilation on the fly.
	// Change literals below to match game code .dll.
	Char sourceGameCodeDLLFullPath[MAX_PATH];
	Win32BuildEXEPathFileName(&win32State, TEXT("TestGame.dll"),
		sourceGameCodeDLLFullPath, sizeof(sourceGameCodeDLLFullPath) / sizeof(Char));

	Char tempGameCodeDLLFullPath[MAX_PATH];
	Win32BuildEXEPathFileName(&win32State, TEXT("TestGame.temp.dll"),
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
		int width = 400, height = 300;
		const Char *windowTitle = TEXT("Test Game");

	#define WINDOW_TOPMOST

		DWORD exStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE
		#if defined WINDOW_TOPMOST
			| WS_EX_TOPMOST
		#endif
			;
		DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

		RECT size = { 0, 0, width, height };
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
			gameMemory.Permanent = VirtualAlloc(0, (size_t) totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			gameMemory.Transient = ((byte *) gameMemory.Permanent + gameMemory.PermanentSize);

			gameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
			gameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
			gameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;

			if (gameMemory.Permanent && gameMemory.Transient)
			{
				GameInput input[2] = {};
				GameInput *newInput = &input[0];
				GameInput *oldInput = &input[1];

				int monitorRefreshHz = 60;
				{
					HDC deviceContext = GetDC(window);
					int regreshRate = GetDeviceCaps(deviceContext, VREFRESH);
					ReleaseDC(window, deviceContext);
					if (regreshRate > 1)
						monitorRefreshHz = regreshRate;
				}
				real32 gameUpdateHz = (monitorRefreshHz / 2.0f);
				real32 targetSecondsPerFrame = (1.0f / (gameUpdateHz));

				// Undef to print sound stats
				// #define SOUND_DEBUG_INFO

				const u32 desiredSchedulerMS = 1;
				const bool hasGrainularSleep = timeBeginPeriod(desiredSchedulerMS) == TIMERR_NOERROR;

				Win32SoundOutput soundOutput;
				ZeroMemory(&soundOutput, sizeof(Win32SoundOutput));
				{
					soundOutput.SamplesPerSecond = 48000;
					soundOutput.BytesPerSample = sizeof(s16) * 2;
					soundOutput.SecondaryBufferSize = soundOutput.SamplesPerSecond * soundOutput.BytesPerSample;
					soundOutput.SafetyBytes = (u32) (((real32) soundOutput.SamplesPerSecond * soundOutput.BytesPerSample / gameUpdateHz) / 3);
				}
				Win32InitDSound(window, soundOutput.SamplesPerSecond, soundOutput.SecondaryBufferSize);
				Win32ClearSoundBuffer(&soundOutput);
				g_SecondarySoundBuffer->Play(0, 0, DSBPLAY_LOOPING);

				s16 *soundSamples = (s16 *) VirtualAlloc(0, soundOutput.SecondaryBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			#if defined SOUND_DEBUG_INFO
				u32 audioLatencyBytes;
				real32 audioLatencySeconds;
			#endif
				bool32 validSound = false;

				Win32Timer frameTimer, frameFlipTimer;

				// Load the game code for the first time
				Win32GameCode game = Win32LoadGameCode(sourceGameCodeDLLFullPath, tempGameCodeDLLFullPath);

				// Window title sugar
				int gameCodeReloadedTitle = 0;

				bool running = true;
				while (running)
				{
					// Check for game code .dll change by comparing the dates, if it is - reload it
					FILETIME newWriteTime = Win32GetLastWriteTime(sourceGameCodeDLLFullPath);

					if (CompareFileTime(&newWriteTime, &game.LastWriteTime) != 0)
					{
						Win32UnloadGameCode(&game);
						game = Win32LoadGameCode(sourceGameCodeDLLFullPath, tempGameCodeDLLFullPath);
						gameCodeReloadedTitle = (int) (gameUpdateHz * 1.5) /* 1.5 seconds */;
					}

					POINT cursorPos;
					GetCursorPos(&cursorPos);
					ScreenToClient(window, &cursorPos);
					newInput->MouseX = cursorPos.x;
					newInput->MouseY = cursorPos.y;
					Win32ProcessKeyboardMessage(&newInput->MouseButtons[0], GetKeyState(VK_LBUTTON) & (1 << 15));
					Win32ProcessKeyboardMessage(&newInput->MouseButtons[1], GetKeyState(VK_MBUTTON) & (1 << 15));
					Win32ProcessKeyboardMessage(&newInput->MouseButtons[2], GetKeyState(VK_RBUTTON) & (1 << 15));
					Win32ProcessKeyboardMessage(&newInput->MouseButtons[3], GetKeyState(VK_XBUTTON1) & (1 << 15));
					Win32ProcessKeyboardMessage(&newInput->MouseButtons[4], GetKeyState(VK_XBUTTON2) & (1 << 15));

					for (int i = 0; i < ArrayCount(newInput->Buttons); ++i)
						newInput->Buttons[i].EndedDown = oldInput->Buttons[i].EndedDown;

					MSG msg;
					while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
					{
						switch (msg.message)
						{
						case WM_QUIT:
						{
							running = false;
						} break;
						case WM_SYSKEYDOWN:
						case WM_SYSKEYUP:
						case WM_KEYDOWN:
						case WM_KEYUP:
						{
							u32 vkCode = (u32) msg.wParam;
							bool wasDown = (msg.lParam & (1 << 30)) != 0;
							bool isDown = (msg.lParam & (1 << 31)) == 0;
							if (wasDown != isDown)
							{
								if (vkCode == 'W')
									Win32ProcessKeyboardMessage(&newInput->MoveUp, isDown);
								else if (vkCode == 'A')
									Win32ProcessKeyboardMessage(&newInput->MoveLeft, isDown);
								else if (vkCode == 'S')
									Win32ProcessKeyboardMessage(&newInput->MoveDown, isDown);
								else if (vkCode == 'D')
									Win32ProcessKeyboardMessage(&newInput->MoveRight, isDown);
							}

							// Alf+F4 handle
							bool32 altKeyWasDown = msg.lParam & (1 << 29);
							if ((vkCode == VK_F4) && altKeyWasDown)
								running = false;
						} break;
						}

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
						screenBuffer.BytesPerPixel = g_BackBuffer.BytesPerPixel;
					}
					if (game.UpdateAndRender)
						game.UpdateAndRender(gameMemory, *newInput, &screenBuffer);

					/* Sound */
					real32 elapsedSecondsToAudio = frameFlipTimer.ElapsedSeconds();

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
						u32 expectedBytesPerFrame = (u32) ((soundOutput.SamplesPerSecond * soundOutput.BytesPerSample) / gameUpdateHz);

						real32 secondsUntilFlip = targetSecondsPerFrame - elapsedSecondsToAudio;
						u32 expectedBytesUntilFlip = (DWORD) ((secondsUntilFlip / targetSecondsPerFrame) * (real32) expectedBytesPerFrame);
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
							game.GetSoundSamples(gameMemory, &soundBuffer);

					#if defined SOUND_DEBUG_INFO
						u32 UnwrappedWriteCursor = writeCursor;
						if (UnwrappedWriteCursor < playCursor)
							UnwrappedWriteCursor += soundOutput.SecondaryBufferSize;
						audioLatencyBytes = (UnwrappedWriteCursor - playCursor);
						audioLatencySeconds = (((real32) audioLatencyBytes / (real32) soundOutput.BytesPerSample) / (real32) soundOutput.SamplesPerSecond);

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
					real32 frameSecondsElapsed = frameTimer.ElapsedSeconds();

					if (frameSecondsElapsed < targetSecondsPerFrame)
					{
						if (hasGrainularSleep)
						{
							u32 sleepMS = (u32) (1000.f * (targetSecondsPerFrame - frameSecondsElapsed));
							if (sleepMS > 1)
								Sleep(sleepMS - 1);
						}
						while (frameSecondsElapsed < targetSecondsPerFrame)
							frameSecondsElapsed = frameTimer.ElapsedSeconds();
					}
					else
					{
						// #Logging Missed a frame!
					}

					real32 msPerFrame = frameTimer.ElapsedMillseconds();
					frameTimer.Reset();

					// Flip here
					Win32WindowDimension dimension = Win32GetWindowDimension(window);
					HDC deviceContext = GetDC(window);
					Win32DisplayBufferInWindow(g_BackBuffer, deviceContext, dimension.Width, dimension.Height);
					ReleaseDC(window, deviceContext);

					frameFlipTimer.Reset();

					// Display stats in window title
					real64 fps = 1000.0f / msPerFrame;

					Char* titleStateDisplay = TEXT("");
					if (gameCodeReloadedTitle-- > 0)
						titleStateDisplay = TEXT("**Game code reloaded** |");
					_stprintf_s(LogMessage, LogMessageSize, TEXT("%s | %s %.02fms/f, %.02ff/s, (%.02fws/f)\n"), windowTitle, titleStateDisplay, msPerFrame, fps, frameSecondsElapsed * 1000.0f);
					SetWindowText(window, LogMessage);

					GameInput *temp = newInput;
					newInput = oldInput;
					oldInput = temp;
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