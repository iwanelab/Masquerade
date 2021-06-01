#pragma once
#include "MovieAccessor.h"
#include "MLProjType.h"
class CZICAccessor
{
private:
	typedef void *(*typezicCreateContext)();
	typedef void (*typezicDestroyContext)(void *zicContext);
	typedef bool (*typezicOpen)(void *zicContext, LPCWSTR path);
	typedef bool (*typezicClose)(void *zicContext);
	typedef int (*typezicGetTotalFrame)(void *zicContext, int icNo);
	typedef int (*typezicGetCameraCount)(void *zicContext);
	typedef int (*typezicGetImageWidth)(void *zicContext, int icNo);
	typedef int (*typezicGetImageHeight)(void *zicContext, int icNo);
	typedef int (*typezicGetImageColorNum)(void *zicContext, int icNo);
	typedef int (*typezicGetSyncICNo)(void *zicContext, int icNo);
	typedef double (*typezicGetFrameRate)(void *zicContext, int icNo);
	typedef bool (*typezicGetFrameBmp)(void *zicContext, int frameNo, void *lpBmpData, int icNo);
	typedef bool (*typezicGetFrameData)(void *zicContext, int frameNo, void *lpBmpData, int icNo);
	typedef void (*typezicSetLicenseFilename)(void *zicContext, LPCWSTR filename);

	typezicCreateContext m_zicCreateContext;
	typezicDestroyContext m_zicDestroyContext;
	typezicOpen m_zicOpen;
	typezicClose m_zicClose;
	typezicGetTotalFrame m_zicGetTotalFrame;
	typezicGetCameraCount m_zicGetCameraCount;
	typezicGetImageWidth m_zicGetImageWidth;
	typezicGetImageHeight m_zicGetImageHeight;
	typezicGetImageColorNum m_zicGetImageColorNum;
	typezicGetSyncICNo m_zicGetSyncICNo;
	typezicGetFrameRate m_zicGetFrameRate;
	typezicGetFrameBmp m_zicGetFrameBmp;
	typezicGetFrameData m_zicGetFrameData;
	typezicSetLicenseFilename m_zicSetLicenseFilename;

protected:
	void* m_context;
	bool m_isOpenMovie;
	HMODULE m_MovieLoader;

public:
	CZICAccessor(void);
	~CZICAccessor(void);

	bool isOpenMovie(){return m_isOpenMovie;}
	bool zicOpen(LPCWSTR path)
	{
		m_isOpenMovie = m_zicOpen(m_context, path);
		return m_isOpenMovie;
	};
	bool zicClose(void) {return m_zicClose(m_context);};
	int zicGetTotalFrame(int icNo) {return m_zicGetTotalFrame(m_context, icNo);};
	int zicGetCameraCount(void) {return m_zicGetCameraCount(m_context);};
	int zicGetImageWidth(int icNo) {return m_zicGetImageWidth(m_context, icNo);};
	int zicGetImageHeight(int icNo) {return m_zicGetImageHeight(m_context, icNo);};
	int zicGetImageColorNum(int icNo) {return m_zicGetImageColorNum(m_context, icNo);};
	double zicGetFrameRate(int icNo) {return m_zicGetFrameRate(m_context, icNo);};
	int zicGetSyncICNo(int icNo) {return m_zicGetSyncICNo(m_context, icNo);};
	bool zicGetFrameBmp(int frameNo, void *lpBmpData, int icNo) {return m_zicGetFrameBmp(m_context, frameNo, lpBmpData, icNo);};
	bool zicGetFrameData(int frameNo, void *lpData, int icNo) {return m_zicGetFrameData(m_context, frameNo, lpData, icNo);};
	void zicSetLicenseFilename(LPCWSTR filename) {return m_zicSetLicenseFilename(m_context, filename);};
	bool isOpenDll(void) {return (m_context != NULL);};
	void *getContext() { return m_context; };
};

