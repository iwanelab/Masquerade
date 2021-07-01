// AVIFile.cpp: CAVIFile �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AVIMovieFile.h"
#include "TCHAR.h"
#pragma comment(lib, "vfw32.lib")

//////////////////////////////////////////////////////////////////////
// �\�z/����
//////////////////////////////////////////////////////////////////////

/// �I�u�W�F�N�g������(����)
LONG CAVIMovieFile::m_ObjectNum = 0;

/**
 * �R���X�g���N�^
 *
 * �I�u�W�F�N�g�̐����A���������s���B
 *
 * @param	�Ȃ�
 *
 * @retval �Ȃ�
 */
CAVIMovieFile::CAVIMovieFile()
	: m_aviFile(NULL), m_aviStm(NULL), m_aviStm2(NULL), m_aviFrm(NULL), m_TotalFrameNum(0)
{
	if (m_ObjectNum <= 0)
	{
		AVIFileInit();
	}
	m_ObjectNum++;
}

/**
 * �f�X�g���N�^
 *
 * �R���X�g���N�^�Ő��������I�u�W�F�N�g�̔j�����s���B
 *
 * @param �Ȃ�
 *
 * @retval �Ȃ�
 */
CAVIMovieFile::~CAVIMovieFile()
{
	close();

	m_ObjectNum--;
	if (m_ObjectNum <= 0)
	{
		AVIFileExit();
	}
}

/**
 * �t�@�C���I�[�v��
 *
 * �����Ŏw�肳�ꂽ�t�@�C�����I�[�v������B
 *
 * @param fileName		�t�@�C����(Input)
 *
 * @retval ��������
 */
BOOL CAVIMovieFile::open(LPCTSTR fileName)
{
	close();

    if (::AVIFileOpen(&m_aviFile, fileName, OF_READ | OF_SHARE_DENY_NONE, NULL) != 0)
        return FALSE;

	if (::AVIFileInfo(m_aviFile, &m_aviInfo, sizeof(m_aviInfo)) != 0)
		return FALSE;

    if (::AVIFileGetStream(m_aviFile, &m_aviStm, streamtypeVIDEO, 0) != 0)
        return FALSE;

	m_TotalFrameNum = ::AVIStreamLength(m_aviStm);

    if ((m_aviFrm = ::AVIStreamGetFrameOpen(m_aviStm, NULL)) == NULL)
        return FALSE;

	return TRUE;
}

/**
 * �t�@�C���N���[�Y
 *
 * ���݃I�[�v�����̃t�@�C�����N���[�Y����B
 *
 * @param �Ȃ�
 *
 * @retval ��������
 */
BOOL CAVIMovieFile::close()
{
	if (m_aviFrm != NULL)
	{
		::AVIStreamGetFrameClose(m_aviFrm);
		m_aviFrm = NULL;
	}

	if (m_aviStm2 != NULL)
	{
		::AVIStreamRelease(m_aviStm2);
		m_aviStm2 = NULL;
	}

	if (m_aviStm != NULL)
	{
		::AVIStreamRelease(m_aviStm);
		m_aviStm = NULL;
	}

	if (m_aviFile != NULL)
	{
		::AVIFileRelease(m_aviFile);
		m_aviFile = NULL;
	}

	return TRUE;
}

/**
 * �t���[���擾
 *
 * �����Ŏw�肳�ꂽ�t���[���ԍ��̉摜���擾����B
 *
 * @param frameNo		�t���[���ԍ�(Input)
 *
 * @retval �t���[���摜��BITMAPINFO�\���̃|�C���^
 */
LPBITMAPINFO CAVIMovieFile::getFrame(const LONG frameNo)
{
	if (m_TotalFrameNum <= frameNo)
	{
		return NULL;
	}

	m_lpBmpInfo = reinterpret_cast<LPBITMAPINFO>(AVIStreamGetFrame(m_aviFrm, frameNo));

	return m_lpBmpInfo;
}

BOOL CAVIMovieFile::chooseCompressor(const LONG totalFrmNum, const LPBITMAPINFOHEADER lpBmpInfoHeader, const LONG fps)
{
	//AVICOMPRESSOPTIONS opt;
	AVISTREAMINFO si =
	 {streamtypeVIDEO, comptypeDIB, 0, 0, 0, 0,
			1, fps, 0, totalFrmNum, 0, 0, (DWORD)-1, 0,
			{0, 0, lpBmpInfoHeader->biWidth, lpBmpInfoHeader->biHeight}, 0, 0, _T("Video #1")};

	m_si = si;

	COMPVARS cv;

	memset(&cv, 0, sizeof(COMPVARS));
	cv.cbSize = sizeof(COMPVARS);
	cv.dwFlags = ICMF_COMPVARS_VALID;
	cv.fccHandler = comptypeDIB;
	cv.lQ = ICQUALITY_DEFAULT;
	if (!::ICCompressorChoose(NULL, ICMF_CHOOSE_DATARATE | ICMF_CHOOSE_KEYFRAME,
														lpBmpInfoHeader, NULL, &cv, NULL))
		return FALSE;

	m_si.fccHandler = cv.fccHandler;
	m_opt.fccType = streamtypeVIDEO;
	m_opt.fccHandler = cv.fccHandler;
	m_opt.dwKeyFrameEvery = cv.lKey;
	m_opt.dwQuality = cv.lQ;
	m_opt.dwBytesPerSecond = cv.lDataRate;
	m_opt.dwFlags = (cv.lDataRate > 0 ? AVICOMPRESSF_DATARATE : 0)
				| (cv.lKey > 0 ? AVICOMPRESSF_KEYFRAMES : 0);
	m_opt.lpFormat = NULL;
	m_opt.cbFormat = 0;
	m_opt.lpParms = cv.lpState;
	m_opt.cbParms = cv.cbState;
	m_opt.dwInterleaveEvery = 0;

	m_lpBmpInfoHeader = lpBmpInfoHeader;

	return TRUE;
}

/**
 * AVI�t�@�C������
 *
 * �����Ŏw�肳�ꂽ�d�l�ŁA�w�薼��AVI�t�@�C���𐶐�����B
 *
 * @param fileName		�t�@�C����(Input)
 * @param totalFrmNum	���t���[����(Input)
 * @param lpBmpInfo		BITMAPINFOHEADER�\���̂̃|�C���^(Input)
 *
 * @retval ��������
 */
BOOL CAVIMovieFile::create(LPCTSTR fileName)//, const LONG totalFrmNum)//, const LPBITMAPINFOHEADER lpBmpInfo)
{
	/*
	AVICOMPRESSOPTIONS opt;
	AVISTREAMINFO si={streamtypeVIDEO, comptypeDIB, 0, 0, 0, 0,
			1, 30, 0, totalFrmNum, 0, 0, (DWORD)-1, 0,
			{0, 0, lpBmpInfo->biWidth, lpBmpInfo->biHeight}, 0, 0, _T("Video #1")};

	COMPVARS cv;

	memset(&cv, 0, sizeof(COMPVARS));
	cv.cbSize = sizeof(COMPVARS);
	cv.dwFlags = ICMF_COMPVARS_VALID;
	cv.fccHandler = comptypeDIB;
	cv.lQ = ICQUALITY_DEFAULT;
	if (!::ICCompressorChoose(NULL, ICMF_CHOOSE_DATARATE | ICMF_CHOOSE_KEYFRAME,
														lpBmpInfo, NULL, &cv, NULL))
		return FALSE;

	si.fccHandler = cv.fccHandler;
	opt.fccType = streamtypeVIDEO;
	opt.fccHandler = cv.fccHandler;
	opt.dwKeyFrameEvery = cv.lKey;
	opt.dwQuality = cv.lQ;
	opt.dwBytesPerSecond = cv.lDataRate;
	opt.dwFlags = (cv.lDataRate > 0 ? AVICOMPRESSF_DATARATE : 0)
				| (cv.lKey > 0 ? AVICOMPRESSF_KEYFRAMES : 0);
	opt.lpFormat = NULL;
	opt.cbFormat = 0;
	opt.lpParms = cv.lpState;
	opt.cbParms = cv.cbState;
	opt.dwInterleaveEvery = 0;
	*/


	if (::AVIFileOpen(&m_aviFile, fileName, OF_CREATE | OF_WRITE | OF_SHARE_DENY_NONE, NULL) != 0)
		return FALSE;

	if (::AVIFileCreateStream(m_aviFile, &m_aviStm, &m_si) != 0)
		return FALSE;

	if (::AVIMakeCompressedStream(&m_aviStm2, m_aviStm, &m_opt, NULL) != AVIERR_OK)
		return FALSE;

	if (::AVIStreamSetFormat(m_aviStm2, 0, m_lpBmpInfoHeader, sizeof(BITMAPINFOHEADER)) !=0)
		return FALSE;

	return TRUE;
}

/**
 * �t���[���摜�ǉ�
 *
 * �����Ŏw�肳�ꂽ�d�l�ŁA�t���[���摜��ǉ�����B
 *
 * @param lpData		�f�[�^�̐擪�A�h���X(Input)
 * @param frameNo		�t���[���ԍ�(Input)
 * @param dataSize		�f�[�^�T�C�Y(Input)
 *
 * @retval ��������
 */
BOOL CAVIMovieFile::addFrameImage(const LPBYTE lpData, const LONG frameNo, const DWORD dataSize, const BOOL isKeyFrame)
{
	DWORD keyFlag = isKeyFrame ? AVIIF_KEYFRAME : 0;
	if (::AVIStreamWrite(m_aviStm2, frameNo, 1, lpData, dataSize, keyFlag, NULL, NULL) != 0)
		return FALSE;

	return TRUE;
}
