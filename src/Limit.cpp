#include "Platform.h"

#include "Win32_FileIO.cpp"

static void GameUpdateAndRender(GameMemory *memory)
{
	GameState *state = (GameState *) memory->Permanent;
	if (!state->IsInitialized)
	{
		/* Code that runs on game intitialization */

		auto[contents, contentsSize] = DEBUGPlatformReadEntireFile(TEXT(__FILE__));
		if (contents)
		{
			DEBUGPlatformWriteEntireFile(TEXT("test.out"), contents, contentsSize);
			DEBUGPlatformFreeFileMemory(contents);
		}

		state->IsInitialized = true;
	}
}