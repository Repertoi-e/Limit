#include "Win32_Limit.h" // for Win32BuildEXEPathFileName

// Gets the input file path relative to the .exe file
// For example if the executable is: L:/bin/Win32_Limit.exe
// This might return: L:/bin/input_recording_0_state.limit
static void Win32GetInputFileLocation(Win32State *state, bool inputStream, int slot, String&& dest)
{
	Char temp[64];
	wsprintf(temp, TEXT("input_recording_%d_%s.limit"), slot, inputStream ? TEXT("input") : TEXT("state"));
	Win32BuildEXEPathFileName(state, temp, (String&&) dest);
}

static void Win32BeginRecordingInput(Win32State *state, int recordingSlot)
{
	Win32ReplayBuffer *buffer = &state->ReplayBuffers[recordingSlot];
	if (buffer->MemoryBlock)
	{
		state->InputRecordingSlot = recordingSlot;

		Char FileName[MAX_PATH];
		Win32GetInputFileLocation(state, true, recordingSlot, String(FileName, MAX_PATH));
		state->RecordingHandle = CreateFile(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

		CopyMemory(buffer->MemoryBlock, state->GameMemoryBlock, state->TotalMemorySize);
	}
}

static void Win32EndRecordingInput(Win32State *state)
{
	CloseHandle(state->RecordingHandle);
	state->RecordingHandle = 0;
	state->InputRecordingSlot = -1;
}

// Write a single frame of input to the file
static void Win32RecordInput(Win32State *state, GameInput *input)
{
	DWORD ignored;
	WriteFile(state->RecordingHandle, input, sizeof(GameInput), &ignored, 0);
}

static void Win32BeginInputPlayBack(Win32State *state, int playingSlot)
{
	Win32ReplayBuffer *buffer = &state->ReplayBuffers[playingSlot];
	if (buffer->MemoryBlock)
	{
		// Save the input before starting playback. See Win32_Limit.h for explanation.
		CopyMemory(state->SavedInputBeforePlay, state->Input, sizeof(state->Input));

		state->InputPlayingSlot = playingSlot;
		Char FileName[MAX_PATH];
		Win32GetInputFileLocation(state, true, playingSlot, String(FileName, MAX_PATH));

		state->PlaybackHandle = CreateFile(FileName, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);

		CopyMemory(state->GameMemoryBlock, buffer->MemoryBlock, state->TotalMemorySize);
	}
}

static void Win32EndInputPlayBack(Win32State *state)
{
	// Restore the state of the input to what it was before start of playback. See Win32_Limit.h for explanation.
	CopyMemory(state->Input, state->SavedInputBeforePlay, sizeof(state->SavedInputBeforePlay));

	CloseHandle(state->PlaybackHandle);
	state->PlaybackHandle = 0;
	state->InputPlayingSlot = -1;
}

// Read a single frame of input from the file and loop if the end is reached
static void Win32PlayBackInput(Win32State *state, GameInput *input)
{
	DWORD bytesRead;
	if (ReadFile(state->PlaybackHandle, input, sizeof(GameInput), &bytesRead, 0))
	{
		if (bytesRead == 0)
		{
			// Loop back to the beginning when we hit the end.
			int inputPlayingSlot = state->InputPlayingSlot; // save the slot
			Win32EndInputPlayBack(state);
			Win32BeginInputPlayBack(state, inputPlayingSlot);
			ReadFile(state->PlaybackHandle, input, sizeof(GameInput), &bytesRead, 0);
		}
	}
}
