// AVIFile.h: CAVIFile クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_AVIFILE_H__21430E6F_DA5A_419E_A163_08F13B7B5FC3__INCLUDED_)
#define AFX_AVIFILE_H__21430E6F_DA5A_419E_A163_08F13B7B5FC3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include "vfw.h"

class CAVIMovieFile 
{
public:
	BOOL addFrameImage(const LPBYTE lpData, const LONG frameNo, const DWORD dataSize, const BOOL isKeyFrame);
	BOOL create(LPCTSTR fileName);//, const LONG totalFrmNum, const LPBITMAPINFOHEADER lpBmpInfo);
	LPBITMAPINFO getFrame(const LONG frameNo);
	BOOL close();
	BOOL open(LPCTSTR fileName);
	CAVIMovieFile();
	virtual ~CAVIMovieFile();

	/// 総フレーム数取得
	LONG getTotalFrameNum()
	{
		return m_TotalFrameNum;
	}

	BOOL chooseCompressor(const LONG totalFrmNum, const LPBITMAPINFOHEADER lpBmpInfo, const LONG fps);

protected:
	/// AVIFILE構造体のポインタ
	PAVIFILE		m_aviFile;

	/// AVISTREAM構造体のポインタ
	PAVISTREAM		m_aviStm;

	/// AVISTREAM構造体のポインタ
	PAVISTREAM		m_aviStm2;

	/// GETFRAME構造体のポインタ
	PGETFRAME		m_aviFrm;

	/// AVIFILEINFO構造体のポインタ
	AVIFILEINFO		m_aviInfo;

	/// BITMAPINFO構造体のポインタ
	LPBITMAPINFO	m_lpBmpInfo;

	/// 総フレーム数
	LONG			m_TotalFrameNum;

	/// オブジェクト生成数(static宣言)
	static LONG m_ObjectNum;


	AVICOMPRESSOPTIONS m_opt;
	AVISTREAMINFO m_si;//={streamtypeVIDEO, comptypeDIB, 0, 0, 0, 0,
			//1, 30, 0, totalFrmNum, 0, 0, (DWORD)-1, 0,
			//{0, 0, lpBmpInfo->biWidth, lpBmpInfo->biHeight}, 0, 0, _T("Video #1")};
	LPBITMAPINFOHEADER m_lpBmpInfoHeader;
};

#endif // !defined(AFX_AVIFILE_H__21430E6F_DA5A_419E_A163_08F13B7B5FC3__INCLUDED_)
