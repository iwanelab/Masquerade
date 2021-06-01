#pragma once
#include <windows.h>
#include <opencv.hpp>

class CProjectionImageAccessor
{
private:
	typedef void *(*typePano2CubeImage)(unsigned char *cubeData, int cubeWidth, int cubeHeight, float cubeFov, int projType,
										unsigned char *panoData, int panoWidth, int panoHeight, int color);
	typedef void *(*typeCube2PanoRect)(	RECT *panoRect, int panoWidth, int panoHeight,
										RECT *cubeRect, int cubeWidth, int cubeHeight, float cubeFov, int projType);
	typePano2CubeImage m_pano2CubeImage;
	typeCube2PanoRect m_cube2PanoRect;
public:
	CProjectionImageAccessor(void);
	~CProjectionImageAccessor(void);
	
	bool pano2CubeImage(unsigned char *cubeData, int cubeWidth, int cubeHeight, float cubeFov, int projType,
						unsigned char *panoData, int panoWidth, int panoHeight, int color)
	{
		return m_pano2CubeImage(cubeData, cubeWidth, cubeHeight, cubeFov, projType,
								panoData, panoWidth, panoHeight, color);
	};
	bool cube2PanoRect(	RECT *cubeRect, int cubeWidth, int cubeHeight, float cubeFov, int projType,
						RECT *panoRect, int panoWidth, int panoHeight)
	{
		return m_cube2PanoRect(	panoRect, panoWidth, panoHeight,
								cubeRect, cubeWidth, cubeHeight, cubeFov, projType);
	};
protected:
	HMODULE m_projectionImage;
};
