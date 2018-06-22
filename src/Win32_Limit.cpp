#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")

#include "Types.h"

// The entire project is built as a single
// compilation unit (this platform file) which
// is a very simple way to handle multiple
// platform support and engine reuse.
// 
// If you want to launch a different "project" (game) with
// the the engine just change the include below.
#include "Limit.cpp"
//
// Every game provides "services" for the platform
// (e.g. GameUpdateAndRender) (Look in Platform.h)
// instead of abstracting window handles and methods, etc.
// (Does the game really need to have any
// idea what a window is or how to manage it?)
//
// The idea is to make it very easy for developers
// to port to another platform. Basically the platform
// layer is supposed to ask the game to do stuff instead
// of the other way around, which creates very complicated
// and hard to understand code.
// The only instance where game code calls stuff from the
// platform layer is for file I/O.
//
// To port to another platform just do all the plaform
// specific stuff in a .cpp (e.g. Mac_Limit.cpp) and
// call game services where appropriate.
// Use this Win32 platform layer as an example.

static LRESULT CALLBACK WindowProc(HWND window, u32 message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
	{
		PostQuitMessage(0);
	} break;
	case WM_PAINT:
	{
		PAINTSTRUCT paint;
		HDC hdc = BeginPaint(window, &paint);

		int width = paint.rcPaint.right - paint.rcPaint.left;
		int height = paint.rcPaint.bottom - paint.rcPaint.top;

		PatBlt(hdc, paint.rcPaint.left, paint.rcPaint.top, width, height, WHITENESS);
		EndPaint(window, &paint);
	} return 0;
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
	return (real32)(end.QuadPart - start.QuadPart) / (real32) g_PerformanceFrequency;
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE, PSTR, int)
{
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
		const Char* windowTitle = TEXT("Limit Test");

		DWORD exStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

		RECT size = { 0, 0, width, height };
		AdjustWindowRectEx(&size, style, false, exStyle);

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
			
			if (gameMemory.Permanent && gameMemory.Transient)
			{
				int displayHz = 60; // #Hardcoded
				int gameUpdateHz = displayHz / 2;
				real32 targetSecondsPerFrame = 1.f / (real32) gameUpdateHz;

				u32 desiredSchedulerMS = 1;
				bool hasGranularSleep = timeBeginPeriod(desiredSchedulerMS) == TIMERR_NOERROR;
				
				LARGE_INTEGER lastCounter = Win32GetWallClock();

				bool running = true;
				while (running)
				{
					MSG msg;
					while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
					{
						if (msg.message == WM_QUIT)
							running = false;
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}

					GameUpdateAndRender(&gameMemory);

					LARGE_INTEGER workCounter = Win32GetWallClock();
					real32 workSecondsElapsed = Win32GetSecondsElapsed(lastCounter, workCounter);

					real32 frameSecondsElapsed = workSecondsElapsed;

					if (frameSecondsElapsed < targetSecondsPerFrame)
					{
						if (hasGranularSleep)
							Sleep((u64) (1000.f * (targetSecondsPerFrame - frameSecondsElapsed)));
						while (frameSecondsElapsed < targetSecondsPerFrame)
							frameSecondsElapsed = Win32GetSecondsElapsed(lastCounter, Win32GetWallClock());
					}
					else
					{
						// #Logging Missed a frame!
					}

					LARGE_INTEGER elapsedCounter = Win32GetWallClock();
					{
						// Display stats in window title
						real64 fps = (real64) g_PerformanceFrequency / (real64) (elapsedCounter.QuadPart - lastCounter.QuadPart);
						real32 msPerFrame = Win32GetSecondsElapsed(lastCounter, elapsedCounter) * 1000.f;
						Char buffer[50];
						_stprintf_s(buffer, sizeof(buffer), TEXT("%s | %.02fms/f,   %.02ff/s\n"), windowTitle, msPerFrame, fps);
						SetWindowText(window, buffer);
					}

					lastCounter = elapsedCounter;
				}
			}
		}
		else
		{
			Char buffer[128];
			_stprintf_s(buffer, sizeof(buffer), TEXT("Terminal failure: Unable to create a window.\n GetLastError=%08x\n"), GetLastError());
			OutputDebugString(buffer);
			return -1;
		}
	}
	else
	{
		Char buffer[128];
		_stprintf_s(buffer, sizeof(buffer), TEXT("Terminal failure: Unable to register internal window class.\n GetLastError=%08x\n"), GetLastError());
		OutputDebugString(buffer);
		return -1;
	}

	return 0;
}
