// AVIFile.h: CAVIFile �N���X�̃C���^�[�t�F�C�X
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

	/// ���t���[�����擾
	LONG getTotalFrameNum()
	{
		return m_TotalFrameNum;
	}

	BOOL chooseCompressor(const LONG totalFrmNum, const LPBITMAPINFOHEADER lpBmpInfo, const LONG fps);

protected:
	/// AVIFILE�\���̂̃|�C���^
	PAVIFILE		m_aviFile;

	/// AVISTREAM�\���̂̃|�C���^
	PAVISTREAM		m_aviStm;

	/// AVISTREAM�\���̂̃|�C���^
	PAVISTREAM		m_aviStm2;

	/// GETFRAME�\���̂̃|�C���^
	PGETFRAME		m_aviFrm;

	/// AVIFILEINFO�\���̂̃|�C���^
	AVIFILEINFO		m_aviInfo;

	/// BITMAPINFO�\���̂̃|�C���^
	LPBITMAPINFO	m_lpBmpInfo;

	/// ���t���[����
	LONG			m_TotalFrameNum;

	/// �I�u�W�F�N�g������(static�錾)
	static LONG m_ObjectNum;


	AVICOMPRESSOPTIONS m_opt;
	AVISTREAMINFO m_si;//={streamtypeVIDEO, comptypeDIB, 0, 0, 0, 0,
			//1, 30, 0, totalFrmNum, 0, 0, (DWORD)-1, 0,
			//{0, 0, lpBmpInfo->biWidth, lpBmpInfo->biHeight}, 0, 0, _T("Video #1")};
	LPBITMAPINFOHEADER m_lpBmpInfoHeader;
};

#endif // !defined(AFX_AVIFILE_H__21430E6F_DA5A_419E_A163_08F13B7B5FC3__INCLUDED_)
