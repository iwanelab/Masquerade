#include <windows.h>
#include <tchar.h>
#include <opencv.hpp>
#include "ExternalDetectorConstants.h"
#include "ExternalDetector.h"
#include <Shlwapi.h>
#include <shlobj.h>

CExternalDetector::CExternalDetector()
: m_hWnd(NULL),
 m_maxDetectedData(2048)
, m_msgCounter(0)
, m_ParentProcess(false)
{
	memset(&m_processInfo, 0, sizeof(PROCESS_INFORMATION));
}

CExternalDetector::~CExternalDetector(void)
{
	if (m_ParentProcess)
	{
		if ((m_processInfo.dwProcessId == 0) || (m_processInfo.dwProcessId == 4))	// System Idle StateÇ∆SystemÇÕèIóπÇµÇ»Ç¢ÅB
			return;
		LRESULT result;
		bool ret = sendProcessMessage(m_processInfo.dwProcessId, result, WM_CLOSE, 0, 0);
		if (!ret || ::WaitForSingleObject(m_processInfo.hProcess, 5000) == WAIT_TIMEOUT)
		{
			TerminateProcess(m_processInfo.hProcess, 0);
		}

		::CloseHandle(m_processInfo.hThread);
		::CloseHandle(m_processInfo.hProcess);
	}
}


bool CExternalDetector::initChildProcess(TCHAR * externalDetectorPath)
{
	STARTUPINFO startupInfo;
	memset(&startupInfo, 0, sizeof(STARTUPINFO));
	memset(&m_processInfo, 0, sizeof(PROCESS_INFORMATION));
	if (!PathFileExists(externalDetectorPath))
	{
		return false;
	}
	if (CreateProcess(externalDetectorPath, NULL, NULL, NULL, TRUE, 0, NULL, NULL, &startupInfo, &m_processInfo) == 0)
	{
		return false;
	}
	m_mapFileNumber = 0;
	m_ParentProcess = true;
	return true;
}


bool CExternalDetector::closeChildProcess(void)
{
	LRESULT retMsg;
	bool ret = sendProcessMessage(m_processInfo.dwProcessId, retMsg, WM_CLOSE, 0, 0);
	if (!ret || (retMsg != 0))
		return false;
	return true;
}

bool CExternalDetector::setParameters(const wchar_t * paramFile)
{
	COPYDATASTRUCT msgData;
	msgData.dwData = detector::datatype_Parameter;
	std::wstring filepath(paramFile);
	msgData.cbData = filepath.size()*sizeof(wchar_t)+1;
	msgData.lpData = &filepath[0];
	//msgData.cbData = (wcslen(paramFile)*sizeof(wchar_t))+1;
	//msgData.lpData = reinterpret_cast<PVOID>(&paramFile);
	LRESULT retMsg;
	bool ret = sendProcessMessage(m_processInfo.dwProcessId, retMsg, WM_COPYDATA, reinterpret_cast<WPARAM>(m_hWnd), reinterpret_cast<LPARAM>(&msgData));
	if (!ret || (retMsg != 0))
		return false;
	return true;
}

bool CExternalDetector::detect(cv::Mat image, std::vector<detector::detectedData> &result)
{
	COPYDATASTRUCT msgData;
	if (!setImage(image, msgData))
	{
		return false;
	}
	detector::detectMessage *pDetectMessage = reinterpret_cast<detector::detectMessage *>(msgData.lpData);
	HANDLE hMapFile = CreateFileMappingA( (HANDLE)(-1), NULL, PAGE_READWRITE, 0,  static_cast<DWORD>(pDetectMessage->mapfileSize), pDetectMessage->mapfileDetectedData);
	if (hMapFile == NULL)
	{
		delete msgData.lpData;
		return false;
	}
	LRESULT retMsg;
	bool ret = sendProcessMessage(m_processInfo.dwProcessId, retMsg, WM_COPYDATA, reinterpret_cast<WPARAM>(m_hWnd), reinterpret_cast<LPARAM>(&msgData));
	if (!ret || (retMsg < 0))
	{
		delete msgData.lpData;
		CloseHandle(hMapFile);
		return false;
	}
	if (retMsg == detector::DETECT_NOT_ENOUGH_MEMORY)
		OutputDebugString(_T("Not Enough Maped Memory"));
	delete msgData.lpData;

	void *lpData = MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 0);
	__int64 *pSizeDetected = static_cast<__int64 *>(lpData);
	detector::detectedData *pDetectedData = reinterpret_cast<detector::detectedData*>(pSizeDetected+1);
	detector::detectedData *ptrData = pDetectedData;
	for (int i = 0; i < *pSizeDetected; ++i, ++ptrData)
		result.push_back(*ptrData);
	if (UnmapViewOfFile(lpData))
	{
		CloseHandle(hMapFile);
	}

	return true;
}

bool CExternalDetector::setImage(cv::Mat image, COPYDATASTRUCT &msgData)
{
	int step;
	if (image.step == cv::Mat::AUTO_STEP)
		step = image.cols * image.elemSize();
	else
		step = image.step;
	msgData.dwData = detector::datatype_Detect;
	msgData.cbData = static_cast<DWORD>(sizeof(detector::detectMessage) + image.rows * step);
	msgData.lpData = new unsigned char [msgData.cbData];
	detector::detectMessage *pMsgHeader = reinterpret_cast<detector::detectMessage *>(msgData.lpData);
	sprintf_s(pMsgHeader->mapfileDetectedData, sizeof(pMsgHeader->mapfileDetectedData), "MAPFILENAME%08d", ++m_mapFileNumber);
	pMsgHeader->mapfileSize = m_maxDetectedData*sizeof(detector::detectedData)+sizeof(__int64);
	pMsgHeader->imageHeader.rows = image.rows;
	pMsgHeader->imageHeader.cols = image.cols;
	pMsgHeader->imageHeader.type = image.type();
	pMsgHeader->imageHeader.step = image.step;
	unsigned char *ptr = pMsgHeader->imageData;
	for(int i= 0; i < image.rows; ++i)
	{
		memcpy(ptr, image.ptr(i), step);
		ptr += step;
	}
	return true;
}

bool CExternalDetector::sendProcessMessage(DWORD dwProcessId, LRESULT &result, unsigned int Msg, WPARAM wParam, LPARAM lParam)
{
	if ((dwProcessId == 0) || (dwProcessId == 4))	// System Idle StateÇ∆SystemÇ…ÇÕëóÇÁÇ»Ç¢ÅB
	{
		return FALSE;
	}
	struct ProcessMessageInfo msgInfo;
	msgInfo.processID = dwProcessId;
	msgInfo.Msg = Msg;
	msgInfo.wParam = wParam;
	msgInfo.lParam = lParam;
	if (::EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&msgInfo)))
	{
		return false;
	}
	result = msgInfo.result;
	return true;
}

BOOL CALLBACK CExternalDetector::EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
	struct ProcessMessageInfo *pMsgInfo = reinterpret_cast<struct ProcessMessageInfo *>(lParam);
	DWORD dwProcessId = 0;
	::GetWindowThreadProcessId(hWnd, &dwProcessId);
	if(pMsgInfo->processID == dwProcessId)
	{
		pMsgInfo->result = ::SendMessage(hWnd, pMsgInfo->Msg, pMsgInfo->wParam, pMsgInfo->lParam);
		return FALSE;
	}
    return TRUE;
}
