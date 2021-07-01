#pragma once
#include "ExternalDetector.h"

class CSecuwatcherMaskDetector
	: CExternalDetector
{
public:
	CSecuwatcherMask();
	virtual ~CSecuwatcherMask(void);

	virtual bool setParameters(secuwatcher_access::detectParam &parameters);
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
};
