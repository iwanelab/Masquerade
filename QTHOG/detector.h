#ifndef __DETECTOR_H__
#define __DETECTOR_H__

#define _USE_MATH_DEFINES
//#include <math.h>
#include <opencv.hpp>

namespace Detector
{
	const int TemplateHalfWidth = 24;
	const int TemplateHalfHeight = 24;
	const int TemplateWidth = TemplateHalfWidth * 2;
	const int TemplateHeight = TemplateHalfHeight * 2;

	void getHoG(cv::Mat &src, std::vector<float> &feat);
	void getHoG_partial(cv::Mat &gradDir, cv::Mat &gradMag, int x, int y, std::vector<float> &feat);
	void calcGrad(cv::Mat &src, cv::Mat &gradDir, cv::Mat &gradMag);

	void getLBP(cv::Mat &src, std::vector<float> &feat);
	void getLBP_partial(cv::Mat &lbpMat, int x, int y, std::vector<float> &feat);
	void extractLBP(cv::Mat &src, cv::Mat &lbpMat);
}   //  namespace Util

#endif // __DETECTOR_H__