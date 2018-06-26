#include "Win32_Limit.h"

inline s64 Win32GetWallClock()
{
	LARGE_INTEGER result;
	QueryPerformanceCounter(&result);
	return result.QuadPart;
}

// Basic wrapper around 
class Win32Timer
{
private:
	static s64 s_PerformanceFrequency;
private:
	s64 m_LastTime = Win32GetWallClock(), m_LastQueried;
public:
	inline real32 ElapsedSeconds()
	{
		m_LastQueried = Win32GetWallClock();
		return (real32) (m_LastQueried - m_LastTime) / (real32) s_PerformanceFrequency;
	}

	inline real32 ElapsedMillseconds()
	{
		return ElapsedSeconds() * 1000.f;
	}

	inline void Reset()
	{
		// #Performance Even though the last query is saved, there is no
		// good way to use it when resetting the timer without introducing
		// too much complexity. Hopefully QueryPerformanceCounter taking 
		// too long won't ever become a problem...
		m_LastTime = Win32GetWallClock();
	}
};

static s64 Win32GetPerformanceFrequency()
{
	LARGE_INTEGER query;
	QueryPerformanceFrequency(&query);
	return query.QuadPart;
}

s64 Win32Timer::s_PerformanceFrequency = Win32GetPerformanceFrequency();
