#include <windows.h>
#include <tchar.h>
#include <opencv.hpp>
#include "SecuwatcherMask.h"
#include <Shlwapi.h>
#include <shlobj.h>

CSecuwatcherMask::CSecuwatcherMask()
: m_hWnd(NULL),
 m_maxDetectedData(2048)
, m_msgCounter(0)
, m_ParentProcess(false)
{
	memset(&m_processInfo, 0, sizeof(PROCESS_INFORMATION));
#ifdef SECUWATCHERMASK_EXE
	for (auto model : m_pModel)
		model = nullptr;
#endif
}

CSecuwatcherMask::CSecuwatcherMask(TCHAR *secuwatcherPath)
: m_hWnd(NULL),
 m_maxDetectedData(2048)
, m_msgCounter(0)
, m_ParentProcess(false)
{
	initChildProcess(secuwatcherPath);
}

bool CSecuwatcherMask::initChildProcess(TCHAR * secuwatcherPath)
{
	STARTUPINFO startupInfo;
	memset(&startupInfo, 0, sizeof(STARTUPINFO));
	memset(&m_processInfo, 0, sizeof(PROCESS_INFORMATION));
#ifdef SECUWATCHERMASK_EXE
	for (auto model : m_pModel)
		model = nullptr;
#endif
	if (!PathFileExists(secuwatcherPath))
	{
		MessageBox(m_hWnd, _T("Not exist SecuwatcherMask execute file"), _T("initChildProcess"), MB_OK);
		return false;
	}
	if (CreateProcess(secuwatcherPath, NULL, NULL, NULL, TRUE, 0, NULL, NULL, &startupInfo, &m_processInfo) == 0)
	{
		MessageBox(m_hWnd, _T("SecuwatcherMask execute error"), _T("initChildProcess"), MB_OK);
		return false;
	}
	m_mapFileNumber = 0;
	m_ParentProcess = true;
	return true;
}

CSecuwatcherMask::~CSecuwatcherMask(void)
{
	if (m_ParentProcess)
	{
		if ((m_processInfo.dwProcessId == 0) || (m_processInfo.dwProcessId == 4))	// System Idle StateとSystemは終了しない。
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

bool CSecuwatcherMask::setParameters(secuwatcher_access::detectParam &parameters)
{
	m_paramDetect = parameters;


	if (m_ParentProcess)
	{
		// モデルファイル有無チェック
		for (int i = 0; i < secuwatcher_access::NUM_OF_MODEL; ++i)
		{	
			if (parameters.model[i].valid)
			{
				if (!PathFileExists(m_paramDetect.model[i].cfgFilename))
				{
					return false;
				}
				if (!PathFileExists(m_paramDetect.model[i].weightFilename))
				{
					return false;
				}
			}
		}
		COPYDATASTRUCT msgData;
		msgData.dwData = secuwatcher_access::datatype_Parameter;
		msgData.cbData = sizeof(secuwatcher_access::detectParam);
		msgData.lpData = static_cast<PVOID>(&parameters);
		LRESULT retMsg;
		bool ret = sendProcessMessage(m_processInfo.dwProcessId, retMsg, WM_COPYDATA, reinterpret_cast<WPARAM>(m_hWnd), reinterpret_cast<LPARAM>(&msgData));
		if (!ret || (retMsg != 0))
			return false;
	}
#ifdef SECUWATCHERMASK_EXE
	else
	{
		for (int i = 0; i < secuwatcher_access::NUM_OF_MODEL; ++i)
		{
			if (m_pModel[i] != nullptr)
			{
				delete m_pModel[i];
				m_pModel[i] = nullptr;
			}

			if (m_paramDetect.model[i].valid)
			{
				if (!PathFileExists(m_paramDetect.model[i].cfgFilename) || !PathFileExists(m_paramDetect.model[i].weightFilename))
					return false;
				try 
				{
					m_pModel[i] = new MSDK_Detector(m_paramDetect.model[i].cfgFilename, m_paramDetect.model[i].weightFilename);
				}
				catch (std::exception &e)
				{
					MessageBoxA(m_hWnd, e.what(), "MSDK_Detector Exception", MB_OK);
					return false;
				}
				catch (...)
				{
					MessageBoxA(m_hWnd, "Unknown error", "MSDK_Detector Exception", MB_OK);
					return false;
				}
			}
		}
	}
#endif
	return true;
}

bool CSecuwatcherMask::detect(cv::Mat image, std::vector<secuwatcher_access::detectedData> &result)
{
	if (m_ParentProcess)
	{
		COPYDATASTRUCT msgData;
		if (!setImage(image, msgData))
			return false;
		secuwatcher_access::detectMessage *pDetectMessage = reinterpret_cast<secuwatcher_access::detectMessage *>(msgData.lpData);
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
		if (retMsg == secuwatcher_access::DETECT_NOT_ENOUGH_MEMORY)
			OutputDebugString(_T("Not Enough Maped Memory"));
		delete msgData.lpData;

		void *lpData = MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 0);
		__int64 *pSizeDetected = static_cast<__int64 *>(lpData);
		secuwatcher_access::detectedData *pDetectedData = reinterpret_cast<secuwatcher_access::detectedData*>(pSizeDetected+1);
		secuwatcher_access::detectedData *ptrData = pDetectedData;
		for (int i = 0; i < *pSizeDetected; ++i, ++ptrData)
			result.push_back(*ptrData);
		if (UnmapViewOfFile(lpData))
		{
			CloseHandle(hMapFile);
		}
	}
#ifdef SECUWATCHERMASK_EXE
	else
	{
		std::vector<secuwatcher_access::eModelType> model_List;
		for (int i = 0; i < static_cast<int>(secuwatcher_access::NUM_OF_MODEL); ++i)
			if (m_pModel[i] != nullptr)
				model_List.push_back(static_cast<secuwatcher_access::eModelType>(i));
		auto result_roi_vec = MSDK_thread_ROI_detectLib(image, m_paramDetect.DL_thresh, m_pModel[secuwatcher_access::MODEL_PERSON], m_pModel[secuwatcher_access::MODEL_PLATE], m_pModel[secuwatcher_access::MODEL_FACE]);
		for (int i = 0; i < result_roi_vec.size(); ++i)
		{
			secuwatcher_access::detectedData resultData;
			resultData.modelType = model_List[i];
			for (auto rectData : result_roi_vec.at(i))
			{
				resultData.rectData.x = rectData.x;
				resultData.rectData.y = rectData.y;
				resultData.rectData.width = rectData.w;
				resultData.rectData.height = rectData.h;
				resultData.rectData.prob = rectData.prob;
				resultData.rectData.obj_id = rectData.obj_id;
				result.push_back(resultData);
			}
		}
		deallocVector();
	}
#endif
	return true;
}

bool CSecuwatcherMask::setImage(cv::Mat image, COPYDATASTRUCT &msgData)
{
	int step;
	if (image.step == cv::Mat::AUTO_STEP)
		step = image.cols * image.elemSize();
	else
		step = image.step;
	msgData.dwData = secuwatcher_access::datatype_Detect;
	msgData.cbData = static_cast<DWORD>(sizeof(secuwatcher_access::detectMessage) + image.rows * step);
	msgData.lpData = new unsigned char [msgData.cbData];
	secuwatcher_access::detectMessage *pMsgHeader = reinterpret_cast<secuwatcher_access::detectMessage *>(msgData.lpData);
	sprintf_s(pMsgHeader->mapfileDetectedData, sizeof(pMsgHeader->mapfileDetectedData), "MAPFILENAME%08d", ++m_mapFileNumber);
	pMsgHeader->mapfileSize = m_maxDetectedData*sizeof(secuwatcher_access::detectedData)+sizeof(__int64);
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

bool CSecuwatcherMask::sendProcessMessage(DWORD dwProcessId, LRESULT &result, unsigned int Msg, WPARAM wParam, LPARAM lParam)
{
	if ((dwProcessId == 0) || (dwProcessId == 4))	// System Idle StateとSystemには送らない。
		return FALSE;
	struct ProcessMessageInfo msgInfo;
	msgInfo.processID = dwProcessId;
	msgInfo.Msg = Msg;
	msgInfo.wParam = wParam;
	msgInfo.lParam = lParam;
	if (::EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&msgInfo)))
		return false;
	result = msgInfo.result;
	return true;
}

BOOL CALLBACK CSecuwatcherMask::EnumWindowsProc(HWND hWnd, LPARAM lParam)
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

#ifdef SECUWATCHERMASK_EXE
int CSecuwatcherMask::OnSetParameters(HWND hWnd, PCOPYDATASTRUCT lpData)
{
	if (!setParameters(*reinterpret_cast<secuwatcher_access::detectParam *>(lpData->lpData)))
		return -1;
	return 0;
}

int CSecuwatcherMask::OnDetect(HWND hWnd, PCOPYDATASTRUCT lpData)
{
	secuwatcher_access::detectMessage *pDetectMessage = static_cast<secuwatcher_access::detectMessage *>(lpData->lpData);
	cv::Mat image(pDetectMessage->imageHeader.rows, pDetectMessage->imageHeader.cols, pDetectMessage->imageHeader.type, pDetectMessage->imageData, pDetectMessage->imageHeader.step);
	std::vector<secuwatcher_access::detectedData> resultData;
	// 検出処理
	if (!detect(image, resultData))
		return secuwatcher_access::DETECT_DETECT_ERROR;
	// メモリマップドファイルオープン
	HANDLE hMapFile = OpenFileMappingA(FILE_MAP_WRITE, FALSE, pDetectMessage->mapfileDetectedData);
	if (hMapFile == NULL)
		return secuwatcher_access::DETECT_MAPFILE_ERROR;
	void *lpOutput = MapViewOfFile(hMapFile, FILE_MAP_WRITE, 0, 0, 0);
	if (lpOutput == NULL)
	{
		CloseHandle(hMapFile);
		return secuwatcher_access::DETECT_MAPFILE_ERROR;
	}
	// 結果をかき出し
	__int64 *pSizeDetected = static_cast<__int64 *>(lpOutput);
	secuwatcher_access::detectedData *pDetectedData = reinterpret_cast<secuwatcher_access::detectedData*>(pSizeDetected+1);
	int result;
	if (resultData.size() > m_maxDetectedData)
	{
		*pSizeDetected = m_maxDetectedData;
		result = secuwatcher_access::DETECT_NOT_ENOUGH_MEMORY;
	}
	else
	{
		*pSizeDetected = resultData.size();
		result = secuwatcher_access::DETECT_SUCCESS;
	}
	for (auto detectData : resultData)
		memcpy(pDetectedData++, &detectData, sizeof(secuwatcher_access::stRectData));
	//memcpy(pDetectedData, &resultData[0], sizeof(secuwatcher_access::stRectData)*(*pSizeDetected));
	// メモリマップドファイルクローズ
	UnmapViewOfFile(lpOutput);
	CloseHandle(hMapFile);
	return result;
}
#endif

