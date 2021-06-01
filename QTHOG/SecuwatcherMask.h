#pragma once
#include "SecuwatcherConstants.h"
#ifdef SECUWATCHERMASK_EXE
#include "SecuwatcherMaskSDK_class.hpp"
#endif

class CSecuwatcherMask
{
public:
	CSecuwatcherMask();
	CSecuwatcherMask(TCHAR *secuwatcherPath);
	~CSecuwatcherMask(void);

#ifdef SECUWATCHERMASK_EXE
	int OnSetParameters(HWND hWnd, PCOPYDATASTRUCT lpData);
	int OnDetect(HWND hWnd, PCOPYDATASTRUCT lpData);
#endif

	bool setParameters(secuwatcher_access::detectParam &parameters);
	bool detect(cv::Mat image, std::vector<secuwatcher_access::detectedData> &result);
	void setMaxDetectedData(size_t maxDetectedData){m_maxDetectedData = maxDetectedData;};
	void setWindowHandle(HWND hWnd)	{m_hWnd = hWnd;};
private:
	struct ProcessMessageInfo {
		DWORD processID;
		unsigned int Msg;
		WPARAM wParam;
		LPARAM lParam;
		LRESULT result;
	};

	bool setImage(cv::Mat image, COPYDATASTRUCT &msgData);
	bool sendProcessMessage(DWORD dwProcessId, LRESULT &result, unsigned int Msg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam);

	// multiprocess
	bool m_ParentProcess;
	PROCESS_INFORMATION m_processInfo;
	HWND m_hWnd;
	unsigned short m_msgCounter;
	unsigned int m_mapFileNumber;
	size_t m_maxDetectedData;

	// parameters
	secuwatcher_access::detectParam m_paramDetect;
#ifdef SECUWATCHERMASK_EXE
	MSDK_Detector *m_pModel[3];
#endif
public:
	bool initChildProcess(TCHAR * secuwatcherPath);
};
