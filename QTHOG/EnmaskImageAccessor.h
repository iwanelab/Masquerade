#pragma once
#include "MovieAccessor.h"
class CEnmaskImageAccessor
{
private:
	typedef void *(*typemaskCreateContext)(LPCWSTR filename);
	typedef void (*typemaskDestroyContext)(void *maskContext);
	typedef bool (*typemaskOpen)(void *maskContext, LPCWSTR dstFilename, void *movieContext);
	typedef void (*typemaskClose)(void *maskContext);
	typedef bool (*typemaskEnmaskImage)(void *maskContext, int frameNo, int cameraNo, RECT rectList[], int rectNum, int imageWidth, int imageHeight);
	typedef void (*typemaskSetBlurSize)(void *maskContext, int blurSize);
	typedef void (*typemaskSetJapegParam)(void *maskContext, int jpegParam);
	typedef void (*typemaskSetFov)(void *maskContext, float fov);

	typemaskCreateContext  m_maskCreateContext;
	typemaskDestroyContext m_maskDestroyContext;
	typemaskOpen           m_maskOpen;
	typemaskClose          m_maskClose;
	typemaskEnmaskImage    m_maskEnmaskImage;
	typemaskSetBlurSize    m_maskSetBlurSize;
	typemaskSetJapegParam  m_maskSetJapegParam;
	typemaskSetFov         m_maskSetFov;

protected:
	void* m_context;
	bool m_isOpenMovie;
	HMODULE m_enmaskImage;

public:
	CEnmaskImageAccessor(LPCWSTR filename);
	~CEnmaskImageAccessor(void);

	bool isOpenMovie(){return m_isOpenMovie;}
	bool maskOpen(LPCWSTR dstDilename, void *movieContext)
	{
		m_isOpenMovie = m_maskOpen(m_context, dstDilename, movieContext);
		return m_isOpenMovie;
	};
	void maskClose(void) {m_maskClose(m_context); m_isOpenMovie = false;};
	bool maskEnmaskImage(int frameNo, int cameraNo, RECT rectList[], int rectNum, int imageWidth, int imageHeight)
				{return m_maskEnmaskImage(m_context, frameNo, cameraNo, rectList, rectNum, imageWidth, imageHeight);};
	void maskSetBlurSize(int blurSize) {m_maskSetBlurSize(m_context, blurSize);};
	void maskSetJapegParam(int jpegParam) {m_maskSetJapegParam(m_context, jpegParam);};
	void maskSetFov(float fov) {m_maskSetFov(m_context, fov);};
};

