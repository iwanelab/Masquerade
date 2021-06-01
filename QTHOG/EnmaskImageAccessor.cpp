#include "EnmaskImageAccessor.h"

#ifdef _DEBUG
	#define DLLNAME_ENMASKIMAGE	L"MovieLoaderD.dll"
#else
	#define DLLNAME_ENMASKIMAGE	L"MovieLoader.dll"
#endif

CEnmaskImageAccessor::CEnmaskImageAccessor(LPCWSTR filename)
	: m_context(NULL)
{
	m_enmaskImage = ::LoadLibraryW(DLLNAME_ENMASKIMAGE);
	if (m_enmaskImage == NULL)
	{
		return;
	}

	m_maskCreateContext =  (typemaskCreateContext)::GetProcAddress(m_enmaskImage, "maskCreateContext");
	m_maskDestroyContext = (typemaskDestroyContext)::GetProcAddress(m_enmaskImage, "maskDestroyContext");
	m_maskOpen =           (typemaskOpen)::GetProcAddress(m_enmaskImage, "maskOpen");
	m_maskClose =          (typemaskClose)::GetProcAddress(m_enmaskImage, "maskClose");
	m_maskEnmaskImage =    (typemaskEnmaskImage)::GetProcAddress(m_enmaskImage, "maskEnmaskImage");
	m_maskSetBlurSize =    (typemaskSetBlurSize)::GetProcAddress(m_enmaskImage, "maskSetBlurSize");
	m_maskSetJapegParam =  (typemaskSetJapegParam)::GetProcAddress(m_enmaskImage, "maskSetJapegParam");
	m_maskSetFov =         (typemaskSetFov)::GetProcAddress(m_enmaskImage, "maskSetFov");

	m_context = m_maskCreateContext(filename);
}


CEnmaskImageAccessor::~CEnmaskImageAccessor(void)
{
	if (m_enmaskImage != NULL)
	{
		if (m_context != NULL)
			m_maskDestroyContext(m_context);
		::FreeLibrary(m_enmaskImage);
	}
}

