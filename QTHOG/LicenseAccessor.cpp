#include "stdafx.h"
#include "LicenseAccessor.h"

#ifdef _DEBUG
	#define DLLNAME_MOVIELOADER	L"MovieLoaderD.dll"
#else
	#define DLLNAME_MOVIELOADER	L"MovieLoader.dll"
#endif


CLicenseAccessor::CLicenseAccessor(void)
{
	if (m_MovieLoader == NULL)
		m_MovieLoader = ::LoadLibraryW(DLLNAME_MOVIELOADER);
	if (m_MovieLoader == NULL)
	{
		DWORD err = GetLastError();
		return;
	}
	m_checkLicense = (typeCheckLicense)::GetProcAddress(m_MovieLoader, "checkLicense");
}


CLicenseAccessor::~CLicenseAccessor(void)
{
}
