#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

#include "Types.h"

#include "Limit.cpp"

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

		DWORD exStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

		RECT size = { 0, 0, width, height };
		AdjustWindowRectEx(&size, style, false, exStyle);

		HWND window = CreateWindowEx(exStyle,
			wndClass.lpszClassName,
			TEXT("Limit Test"),
			style,
			CW_USEDEFAULT, CW_USEDEFAULT,
			size.right + (-size.left), size.bottom + (-size.top),
			NULL, NULL, instance, NULL);
		if (window)
		{
			SetFocus(window);

			GameMemory gameMemory;
			gameMemory.PermanentSize = KiloByte(128);
			gameMemory.TransientSize = KiloByte(256);

			u64 totalSize = gameMemory.PermanentSize + gameMemory.TransientSize;
			gameMemory.Permanent = VirtualAlloc(0, (size_t) totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			gameMemory.Transient = ((byte *) gameMemory.Permanent + gameMemory.PermanentSize);
			
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
			}
		}
		else
		{
			_tprintf(TEXT("Terminal failure: Unable to create a window.\n GetLastError=%08x\n"), GetLastError());
			return -1;
		}
	}
	else
	{
		_tprintf(TEXT("Terminal failure: Unable to register internal window class.\n GetLastError=%08x\n"), GetLastError());
		return -1;
	}

	return 0;
}
