#include "IZICAccessor.h"

#ifdef _DEBUG
	#define DLLNAME_MOVIELOADER	"MovieLoaderD.dll"
#else
	#define DLLNAME_MOVIELOADER	"MovieLoader.dll"
#endif

CIZICAccessor::CIZICAccessor(void)
	: m_context(NULL)
{
	m_MovieLoader = ::LoadLibraryA(DLLNAME_MOVIELOADER);
	if (m_MovieLoader == NULL)
	{
		DWORD err = GetLastError();
		return;
	}
	m_izicCreateContext =    (typeizicCreateContext)::GetProcAddress(m_MovieLoader, "izicCreateContext");
	m_izicDestroyContext =   (typeizicDestroyContext)::GetProcAddress(m_MovieLoader, "izicDestroyContext");
	m_izicOpen =             (typeizicOpen)::GetProcAddress(m_MovieLoader, "izicOpen");
	m_izicClose =            (typeizicClose)::GetProcAddress(m_MovieLoader, "izicClose");
	m_izicGetTotalFrame =    (typeizicGetTotalFrame)::GetProcAddress(m_MovieLoader, "izicGetTotalFrame");
	m_izicGetCameraCount =   (typeizicGetCameraCount)::GetProcAddress(m_MovieLoader, "izicGetCameraCount");
	m_izicGetPerspectiveImageSize =   
							 (typeizicGetPerspectiveImageSize)::GetProcAddress(m_MovieLoader, "izicGetPerspectiveImageSize");
	m_izicGetImageWidth =    (typeizicGetImageWidth)::GetProcAddress(m_MovieLoader, "izicGetImageWidth");
	m_izicGetImageHeight =   (typeizicGetImageHeight)::GetProcAddress(m_MovieLoader, "izicGetImageHeight");
	m_izicGetImageColorNum = (typeizicGetImageColorNum)::GetProcAddress(m_MovieLoader, "izicGetImageColorNum");
	m_izicSetImageWidth =    (typeizicSetImageWidth)::GetProcAddress(m_MovieLoader, "izicSetImageWidth");
	m_izicSetImageColorNum = (typeizicSetImageColorNum)::GetProcAddress(m_MovieLoader, "izicSetImageColorNum");
	m_izicGetFrameBmp =      (typeizicGetFrameBmp)::GetProcAddress(m_MovieLoader, "izicGetFrameBmp");
	m_izicGetFrameData =     (typeizicGetFrameData)::GetProcAddress(m_MovieLoader, "izicGetFrameData");
	m_izicGetICV =			 (typeizicGetICV)::GetProcAddress(m_MovieLoader, "izicGetICV");
	m_izicSaveICVtoFile =	 (typeizicSaveICVtoFile)::GetProcAddress(m_MovieLoader, "izicSaveICVtoFile");
	m_izicSetLicenseFilename = 
							 (typeizicSetLicenseFilename)::GetProcAddress(m_MovieLoader, "izicSetLicenseFilename");

	m_context = m_izicCreateContext();
}


CIZICAccessor::~CIZICAccessor(void)
{
	m_izicDestroyContext(m_context);
}

