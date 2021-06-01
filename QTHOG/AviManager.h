#ifndef AVIMANAGER_H
#define AVIMANAGER_H

#pragma warning(disable: 4819)

#include <opencv.hpp>
#include <QString>
#include "IZICAccessor.h"
#include "ZICAccessor.h"
#include "ProjectionImageAccessor.h"
#include "MLProjType.h"

class AviManager
{
public:
    AviManager();
	~AviManager();

private:
	CvCapture *m_capture;
	CIZICAccessor *m_pIzicFile;
	CZICAccessor *m_pZicFile;

	// 以下はfirstImageCollection より取得されている
	int m_frameCount;
	double m_fps;
	int m_width;
	int m_height;
	int m_perspectiveWidth;
	int m_perspectiveHeight;
	double m_fov;
	QString m_curFileName;
	IplImage *m_frameImage;
	IplImage *m_perspectiveFrameImage;

	// 以下はカメラ切り替えに使用されている
	int m_ICNumber;
	bool m_bForceICNumberChange;

	cv::Mat getPanoImage(int frame, int camNumber);

	CProjectionImageAccessor m_projectionImage;

	std::wstring m_LicenseFileName;

public:
	void closeAvi();
	int openAvi(const QString &filename);
	int getFrameCount() {return m_frameCount;}
	int getWidth() {return m_width;}
	int getPerspectiveHeight() {return m_perspectiveWidth;}
	int getPerspectiveWidth() {return m_perspectiveHeight;}
	int getHeight() {return m_height;}
	double getFps() {return m_fps;}
	double getFov() {return m_fov;}		// 本来はIZICを開いたときに更新すべきだがしていない。よって100固定値。
	cv::Mat getImage(int frame, int camNumber = 0, int proj_type = MOVIELOADER_SPHERE);
	//const unsigned char *getOriginalData(int &datasize, int frame, int camNumber = 0);
	const QString &getCurFileName() {return m_curFileName;}

	// 読み込み専用
	void *getCurrentContext() {if (m_pZicFile == NULL) return NULL; return m_pZicFile->getContext();};

	// カメラ切り替え 失敗したらfalse
	bool changeCamera(int cameraNumber);
	int	getCameraCount()
	{
		if(m_pZicFile != NULL)
		{
			return m_pZicFile->zicGetCameraCount();
		}
		else if(m_pIzicFile != NULL)
		{
			return m_pIzicFile->izicGetCameraCount();
		}

		return 1;
	}
	// ライセンスチェックを動画アクセス時に実行する
	void setLicenseFilename(LPCWSTR LicenseFileName) { m_LicenseFileName = std::wstring(LicenseFileName); };
};

#endif // AVIMANAGER_H
