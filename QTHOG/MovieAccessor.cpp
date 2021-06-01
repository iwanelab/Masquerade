#include "MovieAccessor.h"

#ifdef _DEBUG
	#define DLLNAME_MOVIELOADER	L"MovieLoaderD.dll"
#else
	#define DLLNAME_MOVIELOADER	L"MovieLoader.dll"
#endif


CMovieAccessor::CMovieAccessor(void)
:m_MovieLoader(NULL)
{
	if (m_MovieLoader == NULL)
		m_MovieLoader = ::LoadLibraryW(DLLNAME_MOVIELOADER);
	if (m_MovieLoader == NULL)
	{
		DWORD err = GetLastError();
		WCHAR msg[256];
		wsprintfW(msg, L"LoadLibrary Error = %02X\n", err);
		OutputDebugStringW(msg);
	}
}


CMovieAccessor::~CMovieAccessor(void)
{
	if (m_MovieLoader != NULL)
		::FreeLibrary(m_MovieLoader);
}
