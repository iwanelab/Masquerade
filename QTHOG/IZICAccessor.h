#pragma once
#include "MovieAccessor.h"
#include "MLProjType.h"
class CIZICAccessor
{
private:
	typedef void *(*typeizicCreateContext)();
	typedef void (*typeizicDestroyContext)(void *izicContext);
	typedef bool (*typeizicOpen)(void *izicContext, LPCWSTR path, HWND hWorkWindow);
	typedef bool (*typeizicClose)(void *izicContext);
	typedef int (*typeizicGetTotalFrame)(void *izicContext, int icNo);
	typedef int (*typeizicGetCameraCount)(void *izicContext);
	typedef int (*typeizicGetPerspectiveImageSize)(void *izicContext, int icNo);
	typedef int (*typeizicGetImageWidth)(void *izicContext, int icNo);
	typedef int (*typeizicGetImageHeight)(void *izicContext, int icNo);
	typedef int (*typeizicGetImageColorNum)(void *izicContext, int icNo);
	typedef bool (*typeizicSetImageWidth)(void *izicContext, int value, int icNo);
	typedef bool (*typeizicSetImageColorNum)(void *izicContext, int value, int icNo);
	typedef bool (*typeizicGetFrameBmp)(void *izicContext, int frameNo, int type, void *lpBmpData, int icNo);
	typedef bool (*typeizicGetFrameData)(void *izicContext, int frameNo, int type, void *lpBmpData, int icNo);
	typedef size_t (*typeizicGetICV)(void *izicContext, void *lpBuffer, size_t size);
	typedef bool (*typeizicSaveICVtoFile)(void *izicContext, LPCWSTR filename);
	typedef void (*typeizicSetLicenseFilename)(void *izicContext, LPCWSTR filename);

	typeizicCreateContext m_izicCreateContext;
	typeizicDestroyContext m_izicDestroyContext;
	typeizicOpen m_izicOpen;
	typeizicClose m_izicClose;
	typeizicGetTotalFrame m_izicGetTotalFrame;
	typeizicGetCameraCount m_izicGetCameraCount;
	typeizicGetPerspectiveImageSize m_izicGetPerspectiveImageSize;
	typeizicGetImageWidth m_izicGetImageWidth;
	typeizicGetImageHeight m_izicGetImageHeight;
	typeizicGetImageColorNum m_izicGetImageColorNum;
	typeizicSetImageWidth m_izicSetImageWidth;
	typeizicSetImageColorNum m_izicSetImageColorNum;
	typeizicGetFrameBmp m_izicGetFrameBmp;
	typeizicGetFrameData m_izicGetFrameData;
	typeizicGetICV m_izicGetICV;
	typeizicSaveICVtoFile m_izicSaveICVtoFile;
	typeizicSetLicenseFilename m_izicSetLicenseFilename;

protected:
	void* m_context;
	bool m_isOpenMovie;
	HMODULE m_MovieLoader;

public:
	CIZICAccessor(void);
	~CIZICAccessor(void);

	bool isOpenMovie(){return m_isOpenMovie;}
	bool izicOpen(LPCWSTR path, HWND hWorkWindow)
	{
		m_isOpenMovie = m_izicOpen(m_context, path, hWorkWindow);
		return m_isOpenMovie;
	};
	bool izicClose(void) {return m_izicClose(m_context);};
	int izicGetTotalFrame(int icNo) {return m_izicGetTotalFrame(m_context, icNo);};
	int izicGetCameraCount(void) {return m_izicGetCameraCount(m_context);};
	int izicGetPerspectiveImageSize(int icNo) {return m_izicGetPerspectiveImageSize(m_context, icNo);};
	int izicGetImageWidth(int icNo) {return m_izicGetImageWidth(m_context, icNo);};
	int izicGetImageHeight(int icNo) {return m_izicGetImageHeight(m_context, icNo);};
	int izicGetImageColorNum(int icNo) {return m_izicGetImageColorNum(m_context, icNo);};
	bool izicSetImageWidth(int value, int icNo) {return m_izicSetImageWidth(m_context, value, icNo);};
	bool izicSetImageColorNum(int value, int icNo) {return m_izicSetImageColorNum(m_context, value, icNo);};
	bool izicGetFrameBmp(int frameNo, void *lpBmpData, int type, int icNo) {return m_izicGetFrameBmp(m_context, frameNo, type, lpBmpData, icNo);};
	bool izicGetFrameBmp(int frameNo, void *lpBmpData, int icNo) {return izicGetFrameBmp(frameNo, lpBmpData, MOVIELOADER_SPHERE, icNo);};
	bool izicGetFrameData(int frameNo, void *lpData, int type, int icNo) {return m_izicGetFrameData(m_context, frameNo, type, lpData, icNo);};
	bool izicGetFrameData(int frameNo, void *lpData, int icNo) {return izicGetFrameData(frameNo, lpData, MOVIELOADER_SPHERE, icNo);};
	size_t izicGetICV(void *lpBuffer, size_t size) {return m_izicGetICV(m_context, lpBuffer, size);};
	bool izicSaveICVtoFile(LPCWSTR filename) {return m_izicSaveICVtoFile(m_context, filename);};
	void izicSetLicenseFilename(LPCWSTR filename) {return m_izicSetLicenseFilename(m_context, filename);};
	bool isOpenDll(void) {return (m_context != NULL);};
};

