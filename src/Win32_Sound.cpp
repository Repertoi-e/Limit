#include "Win32_Sound.h"

#include "Platform.h"
#include "Limit.h"

#include <Mmreg.h>
#include <dsound.h>

static LPDIRECTSOUNDBUFFER g_SecondarySoundBuffer;

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(DirectSoundCreateFunc);

static void Win32InitDSound(HWND Window, s32 SamplesPerSecond, s32 BufferSize)
{
	// Load the library
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
	if (DSoundLibrary)
	{
		DirectSoundCreateFunc *directSoundCreate = (DirectSoundCreateFunc *) GetProcAddress(DSoundLibrary, "DirectSoundCreate");

		LPDIRECTSOUND directSound;
		if (directSoundCreate && SUCCEEDED(directSoundCreate(0, &directSound, 0)))
		{
			WAVEFORMATEX waveFormat;
			ZeroMemory(&waveFormat, sizeof(WAVEFORMATEX));
			{
				waveFormat.wFormatTag = WAVE_FORMAT_PCM;
				waveFormat.nChannels = 2;
				waveFormat.nSamplesPerSec = SamplesPerSecond;
				waveFormat.wBitsPerSample = 16;
				waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
				waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
			}

			// Buffer desc for primary buffer
			DSBUFFERDESC bufferDesc;
			ZeroMemory(&bufferDesc, sizeof(DSBUFFERDESC));
			{
				bufferDesc.dwSize = sizeof(DSBUFFERDESC);
				bufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
			}

			if (SUCCEEDED(directSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
			{
				// Create a primary buffer
				// #TODO DSBCAPS_GLOBALFOCUS
				LPDIRECTSOUNDBUFFER primaryBuffer;
				if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDesc, &primaryBuffer, 0)))
				{
					HRESULT error = primaryBuffer->SetFormat(&waveFormat);
					if (SUCCEEDED(error))
					{
						OutputDebugString(TEXT("Primary buffer format was set.\n"));
					}
					else
					{
						// #Diagnostics
					}
				}
				else
				{
					// #Diagnostics
				}
			}
			else
			{
				// #Diagnostics
			}

			// Buffer desc for secondary buffer
			{
				bufferDesc.dwSize = sizeof(bufferDesc);
				bufferDesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
				bufferDesc.dwBufferBytes = BufferSize;
				bufferDesc.lpwfxFormat = &waveFormat;
			}

			// Create a secondary buffer
			HRESULT error = directSound->CreateSoundBuffer(&bufferDesc, &g_SecondarySoundBuffer, 0);
			if (SUCCEEDED(error))
			{
				OutputDebugString(TEXT("Secondary buffer created successfully.\n"));
			}
			else
			{
				// #Diagnostics
			}
		}
		else
		{
			// #Diagnostics
		}
	}
	else
	{
		// #Diagnostics
	}
}

static void Win32ClearSoundBuffer(Win32SoundOutput *soundOutput)
{
	void *Region1, *Region2;
	DWORD Region1Size, Region2Size;
	if (SUCCEEDED(g_SecondarySoundBuffer->Lock(0, soundOutput->SecondaryBufferSize, &Region1, &Region1Size, &Region2, &Region2Size, 0)))
	{
		// #TODO Assert that Region1Size/Region2Size is valid
		byte *destSample = (byte *) Region1;
		for (u32 i = 0; i < Region1Size; i++)
			*destSample++ = 0;

		destSample = (byte *) Region2;
		for (u32 i = 0; i < Region2Size; i++)
			*destSample++ = 0;

		g_SecondarySoundBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

static void Win32FillSoundBuffer(Win32SoundOutput& soundOutput, u32 byteToLock, u32 bytesToWrite, const GameSoundOutputBuffer& sourceBuffer)
{
	void *Region1, *Region2;
	DWORD Region1Size, Region2Size;
	if (SUCCEEDED(g_SecondarySoundBuffer->Lock(byteToLock, bytesToWrite, &Region1, &Region1Size, &Region2, &Region2Size, 0)))
	{
		// #TODO Assert that Region1Size/Region2Size is valid

		u32 region1SampleCount = Region1Size / soundOutput.BytesPerSample;
		s16 *destSample = (s16 *) Region1;
		s16 *sourceSample = sourceBuffer.Samples;
		for (DWORD i = 0; i < region1SampleCount; ++i)
		{
			*destSample++ = *sourceSample++;
			*destSample++ = *sourceSample++;
			++soundOutput.RunningSampleIndex;
		}

		u32 region2SampleCount = Region2Size / soundOutput.BytesPerSample;
		destSample = (s16 *) Region2;
		for (DWORD i = 0; i < region2SampleCount; ++i)
		{
			*destSample++ = *sourceSample++;
			*destSample++ = *sourceSample++;
			++soundOutput.RunningSampleIndex;
		}

		g_SecondarySoundBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}
