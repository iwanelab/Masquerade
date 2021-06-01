#include "ZICAccessor.h"

#ifdef _DEBUG
	#define DLLNAME_MOVIELOADER	L"MovieLoaderD.dll"
#else
	#define DLLNAME_MOVIELOADER	L"MovieLoader.dll"
#endif

CZICAccessor::CZICAccessor(void)
	: m_context(NULL)
{
	m_MovieLoader = ::LoadLibraryW(DLLNAME_MOVIELOADER);
	if (m_MovieLoader == NULL)
		return;
	m_zicCreateContext =    (typezicCreateContext)::GetProcAddress(m_MovieLoader, "zicCreateContext");
	m_zicDestroyContext =   (typezicDestroyContext)::GetProcAddress(m_MovieLoader, "zicDestroyContext");
	m_zicOpen =             (typezicOpen)::GetProcAddress(m_MovieLoader, "zicOpen");
	m_zicClose =            (typezicClose)::GetProcAddress(m_MovieLoader, "zicClose");
	m_zicGetTotalFrame =    (typezicGetTotalFrame)::GetProcAddress(m_MovieLoader, "zicGetTotalFrame");
	m_zicGetCameraCount =   (typezicGetCameraCount)::GetProcAddress(m_MovieLoader, "zicGetCameraCount");
	m_zicGetImageWidth =    (typezicGetImageWidth)::GetProcAddress(m_MovieLoader, "zicGetImageWidth");
	m_zicGetImageHeight =   (typezicGetImageHeight)::GetProcAddress(m_MovieLoader, "zicGetImageHeight");
	m_zicGetImageColorNum = (typezicGetImageColorNum)::GetProcAddress(m_MovieLoader, "zicGetImageColorNum");
	m_zicGetFrameRate =     (typezicGetFrameRate)::GetProcAddress(m_MovieLoader, "zicGetFrameRate");
	m_zicGetSyncICNo =      (typezicGetSyncICNo)::GetProcAddress(m_MovieLoader, "zicGetSyncICNo");
	m_zicGetFrameBmp =      (typezicGetFrameBmp)::GetProcAddress(m_MovieLoader, "zicGetFrameBmp");
	m_zicGetFrameData =     (typezicGetFrameData)::GetProcAddress(m_MovieLoader, "zicGetFrameData");
	m_zicSetLicenseFilename = 
							(typezicSetLicenseFilename)::GetProcAddress(m_MovieLoader, "zicSetLicenseFilename");

	m_context = m_zicCreateContext();
}


CZICAccessor::~CZICAccessor(void)
{
	m_zicDestroyContext(m_context);
}

