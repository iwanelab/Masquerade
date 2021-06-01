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

	// �ȉ���firstImageCollection ���擾����Ă���
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

	// �ȉ��̓J�����؂�ւ��Ɏg�p����Ă���
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
	double getFov() {return m_fov;}		// �{����IZIC���J�����Ƃ��ɍX�V���ׂ��������Ă��Ȃ��B�����100�Œ�l�B
	cv::Mat getImage(int frame, int camNumber = 0, int proj_type = MOVIELOADER_SPHERE);
	//const unsigned char *getOriginalData(int &datasize, int frame, int camNumber = 0);
	const QString &getCurFileName() {return m_curFileName;}

	// �ǂݍ��ݐ�p
	void *getCurrentContext() {if (m_pZicFile == NULL) return NULL; return m_pZicFile->getContext();};

	// �J�����؂�ւ� ���s������false
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
	// ���C�Z���X�`�F�b�N�𓮉�A�N�Z�X���Ɏ��s����
	void setLicenseFilename(LPCWSTR LicenseFileName) { m_LicenseFileName = std::wstring(LicenseFileName); };
};

#endif // AVIMANAGER_H
