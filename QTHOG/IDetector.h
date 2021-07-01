#pragma once
#include "RegionData.h"
#include "DetectData.h"
class IDetector
{
public:
	IDetector() {};
	virtual ~IDetector(void) {};

	virtual bool initChildProcess(TCHAR * secuwatcherPath) = 0;
	virtual bool closeChildProcess(void) = 0;
	virtual bool setParameters(const wchar_t * paramFile) = 0;
	virtual bool detect(cv::Mat image, std::vector<detector::detectedData> &result) = 0;
	virtual void setMaxDetectedData(size_t maxDetectedData) = 0;
	virtual void setWindowHandle(HWND hWnd) = 0;
};
