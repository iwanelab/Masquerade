#pragma once
#include <windows.h>

class CMovieAccessor
{
public:
	CMovieAccessor(void);
	~CMovieAccessor(void);
protected:
	HMODULE m_MovieLoader;
};
