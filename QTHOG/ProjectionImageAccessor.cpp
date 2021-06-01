#include "ProjectionImageAccessor.h"

#ifdef _DEBUG
	#define DLLNAME_PROJECTIONIMAGE	L"MovieLoaderD.dll"
#else
	#define DLLNAME_PROJECTIONIMAGE	L"MovieLoader.dll"
#endif


CProjectionImageAccessor::CProjectionImageAccessor(void)
{
	m_projectionImage = ::LoadLibrary(DLLNAME_PROJECTIONIMAGE);
	if (m_projectionImage == NULL)
	{
		//MessageBox(NULL, _T("LoadLibrary Error"), NULL, MB_OK);
		return;
	}
	m_pano2CubeImage = (typePano2CubeImage)::GetProcAddress(m_projectionImage, "pano2CubeImage");
	m_cube2PanoRect = (typeCube2PanoRect)::GetProcAddress(m_projectionImage, "cube2PanoRect");
}


CProjectionImageAccessor::~CProjectionImageAccessor(void)
{
	if (m_projectionImage != NULL)
		::FreeLibrary(m_projectionImage);
}
