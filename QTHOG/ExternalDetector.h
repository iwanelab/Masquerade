#pragma once
#include "DetectData.h"
#include "IDetector.h"

class CExternalDetector
	: public IDetector
{
public:
	CExternalDetector();
	virtual ~CExternalDetector(void);

	virtual bool initChildProcess(TCHAR * externalDetectorPath);
	virtual bool closeChildProcess(void);
	virtual bool setParameters(const wchar_t * paramFile);
	virtual bool detect(cv::Mat image, std::vector<detector::detectedData> &result);
	virtual void setMaxDetectedData(size_t maxDetectedData){m_maxDetectedData = maxDetectedData;};
	virtual void setWindowHandle(HWND hWnd)	{m_hWnd = hWnd;};
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
};