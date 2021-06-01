#include "AviManager.h"
#include <QFileInfo>
#include "tchar.h"
//#include "SphereProjector.h"

AviManager::AviManager()
	: m_capture(0),
	m_frameCount(0),
	m_width(0),
	m_height(0),
	m_curFileName(""),
	m_pZicFile(NULL),
	m_pIzicFile(NULL),
	m_frameImage(0),
	m_ICNumber(0),
	m_bForceICNumberChange(false),
	m_perspectiveWidth(0),
	m_perspectiveHeight(0),
	m_perspectiveFrameImage(0),
	m_fov(100)
{
}

AviManager::~AviManager()
{
	if (m_capture)
		cvReleaseCapture(&m_capture);
	if (m_frameImage)
		cvReleaseImage(&m_frameImage);
	if (m_perspectiveFrameImage)
		cvReleaseImage(&m_perspectiveFrameImage);

	if (m_pZicFile)
	{
		m_pZicFile->zicClose();
		delete m_pZicFile;
		m_pZicFile = NULL;
	}

	if(m_pIzicFile != NULL)
	{
		m_pIzicFile->izicClose();
		delete m_pIzicFile;
		m_pIzicFile = NULL;
	}
}

void AviManager::closeAvi()
{
	if (m_pZicFile)
	{
		m_pZicFile->zicClose();
		delete m_pZicFile;
		m_pZicFile = NULL;
	}

	if(m_pIzicFile != NULL)
	{
		m_pIzicFile->izicClose();
		delete m_pIzicFile;
		m_pIzicFile = NULL;
	}
}

int AviManager::openAvi(const QString &filename)
{
	if (m_capture)
	{
		cvReleaseCapture(&m_capture);
		m_capture = 0;
	}

	if (m_pZicFile)
	{
		m_pZicFile->zicClose();
		delete m_pZicFile;
		m_pZicFile = 0;
	}
	
	if(m_pIzicFile != NULL)
	{
		m_pIzicFile->izicClose();
		delete m_pIzicFile;
		m_pIzicFile = NULL;
	}

	m_curFileName = filename;

	QFileInfo fileInfo(filename);
	QString fileExt = fileInfo.suffix();

if (fileExt == "zic")
	{
		m_pZicFile = new CZICAccessor;
		if (m_pZicFile == NULL)
			return -1;
		m_pZicFile->zicSetLicenseFilename(m_LicenseFileName.c_str());
		if (!m_pZicFile->zicOpen((const wchar_t*)filename.utf16()))
		{
			delete m_pZicFile;
			m_pZicFile = NULL;
			return -1;
		}
		m_frameCount = m_pZicFile->zicGetTotalFrame(0);
		m_fps = m_pZicFile->zicGetFrameRate(0);
		m_width = m_pZicFile->zicGetImageWidth(0);
		m_height = m_pZicFile->zicGetImageHeight(0);
		m_perspectiveWidth = (m_width*100/360+511)/512*512;
		m_perspectiveHeight = m_perspectiveWidth;
	}
	else if(fileExt == "izic")
	{
		m_pIzicFile = new CIZICAccessor();
		if (m_pIzicFile == NULL)
			return -1;
		m_pIzicFile->izicSetLicenseFilename(m_LicenseFileName.c_str());
		if (!m_pIzicFile->izicOpen((const wchar_t*)filename.utf16(), NULL))
		{
			delete m_pIzicFile;
			m_pIzicFile = NULL;
			return -1;
		}
		m_frameCount = m_pIzicFile->izicGetTotalFrame(0);
		m_fps = 0; // AVIでのみ必要みたいだが...
		m_width = m_pIzicFile->izicGetImageWidth(0);
		m_height = m_pIzicFile->izicGetImageHeight(0);
		m_perspectiveWidth = m_pIzicFile->izicGetPerspectiveImageSize(0);
		m_perspectiveHeight = m_perspectiveWidth;
	}
	return m_frameCount;
}



//ここにカメラ番号渡す口を作る作業が必要
cv::Mat AviManager::getPanoImage(int frame, int camNumber)
{
	if (m_pZicFile)
	{
		// カメラ番号更新
		int camnum = camNumber;
		if( m_bForceICNumberChange )
		{
			camnum = m_ICNumber;
		}
		else
		{
			int totalCameraNum = this->m_pZicFile->zicGetCameraCount();
			if(camnum >= totalCameraNum)
			{
				return cv::Mat();
			}
		}
		
		int bmpWidth = abs(m_pZicFile->zicGetImageWidth(camnum));
		int bmpHeight = abs(m_pZicFile->zicGetImageHeight(camnum));
		int bmpColor = abs(m_pZicFile->zicGetImageColorNum(camnum));

		cv::Mat retImage;
		switch(bmpColor)
		{
		case 3:
			retImage = cv::Mat(bmpHeight, bmpWidth, CV_8UC3, cv::Scalar::all(0));
			break;
		case 4:
			retImage = cv::Mat(bmpHeight, bmpWidth, CV_8UC4, cv::Scalar::all(0));
			break;
		default:
			return cv::Mat();
		}

		if(m_pZicFile->zicGetFrameData(frame, retImage.data, camnum) == false)
		{
			return cv::Mat();
		}
		return retImage;
	}
	else if(m_pIzicFile != NULL)
	{
		// カメラ番号更新
		int camnum = camNumber;
		if( m_bForceICNumberChange )
		{
			camnum = m_ICNumber;
		}
		else
		{
			int totalCameraNum = this->m_pIzicFile->izicGetCameraCount();
			if(camnum >= totalCameraNum)
			{
				return cv::Mat();
			}
		}
		
		int bmpWidth = abs(m_pIzicFile->izicGetImageWidth(camnum));
		int bmpHeight = abs(m_pIzicFile->izicGetImageHeight(camnum));
		int bmpColor = abs(m_pIzicFile->izicGetImageColorNum(camnum));

		cv::Mat retImage;
		switch(bmpColor)
		{
		case 3:
			retImage = cv::Mat(bmpHeight, bmpWidth, CV_8UC3, cv::Scalar::all(0));
			break;
		case 4:
			retImage = cv::Mat(bmpHeight, bmpWidth, CV_8UC4, cv::Scalar::all(0));
			break;
		default:
			return cv::Mat();
		}

		if(m_pIzicFile->izicGetFrameData(frame, retImage.data, camnum) == false)
		{
			return cv::Mat();
		}
		return retImage;
	}
}

cv::Mat AviManager::getImage(int frame, int camNumber, int proj_type)
{
	if (proj_type == MOVIELOADER_SPHERE)
	{
		cv::Mat panoImage = getPanoImage(frame, camNumber);
		return panoImage;
	}
	if (m_capture || m_pZicFile)
	{
		cv::Mat panoImage = getPanoImage(frame, camNumber);
		// 平面展開
		if (panoImage.empty())
		{
			return cv::Mat();
		}
		int imageType;
		switch(panoImage.channels())
		{
		case 3:
			imageType = CV_8UC3;
			break;
		case 4:
			imageType = CV_8UC4;
			break;
		default:
			return cv::Mat();
		}
		cv::Mat cubeImage(m_perspectiveWidth, m_perspectiveHeight, imageType, cv::Scalar::all(0));
		if (!m_projectionImage.pano2CubeImage(	cubeImage.data, cubeImage.cols, cubeImage.rows, m_fov, proj_type,
												panoImage.data, panoImage.cols, panoImage.rows, panoImage.channels()))
		{
			return cv::Mat();
		}
		return cubeImage;
	}
	else if (m_pIzicFile != NULL)
	{
		// カメラ番号更新
		int camnum = camNumber;
		if( m_bForceICNumberChange )
		{
			camnum = m_ICNumber;
		}
		else
		{
			int totalCameraNum = this->m_pIzicFile->izicGetCameraCount();
			if(camnum >= totalCameraNum)
			{
				return cv::Mat();
			}
		}
		
		int bmpWidth = abs(m_pIzicFile->izicGetPerspectiveImageSize(camnum));
		int bmpHeight = bmpWidth;
		int bmpColor = abs(m_pIzicFile->izicGetImageColorNum(camnum));

		int imageType;
		switch(bmpColor)
		{
		case 3:
			imageType = CV_8UC3;
			break;
		case 4:
			imageType = CV_8UC4;
			break;
		default:
			return cv::Mat();
		}
		cv::Mat cubeImage(bmpWidth, bmpHeight, imageType, cv::Scalar::all(0));

		if(m_pIzicFile->izicGetFrameData(frame, cubeImage.data, proj_type, camnum) == false)
		{
			return cv::Mat();
		}
		return cubeImage;
	}
}
//ぼかし処理修正時に追加した生データ取得用
//const unsigned char *AviManager::getOriginalData(int &datasize, int frame, int camNumber)
//{
//	if (0 == m_pZicFile) return NULL;
//	
//	int camnum = camNumber;
//	if( m_bForceICNumberChange )
//	{
//		camnum = m_ICNumber;
//	}
//	else
//	{
//		//貰ったカメラ番号が存在しているかチェック（高負荷時は事前チェック型に改修）
//		bool chk = false;
//		std::vector<int> vecICNumbers = m_pZicFile->getSyncICNumbers();
//		std::vector<int>::iterator iterICNumbers = vecICNumbers.begin();
//		for(; iterICNumbers != vecICNumbers.end(); ++iterICNumbers)
//		{
//			if( (*iterICNumbers) == camnum )
//			{
//				chk = true;
//				break;
//			}
//		}
//		if( !chk ) return NULL;
//	}
//
//	std::vector<zic::IImageCollection*> vecIICol = m_pZicFile->queryImageCollections(camnum);
//	return vecIICol.front()->getFrameData(frame,datasize);
//}
//
// カメラ切り替え 失敗したらfalse
bool AviManager::changeCamera(int cameraNumber)
{
	bool bret = false;

	if(this->m_pZicFile != NULL)
	{
		if (this->m_pZicFile->zicGetSyncICNo(cameraNumber) >= 0)
		{
			bret = true;
			m_bForceICNumberChange = true;
			m_ICNumber = cameraNumber;
		}
		//std::vector<int> vecICNumbers = m_pZicFile->getSyncICNumbers();
		//std::vector<int>::iterator iterICNumbers = vecICNumbers.begin();
		//for(; iterICNumbers != vecICNumbers.end(); ++iterICNumbers)
		//{
		//	if( *iterICNumbers == cameraNumber )
		//	{
		//		bret = true;
		//		m_bForceICNumberChange = true;
		//		m_ICNumber = cameraNumber;
		//		break;
		//	}
		//}
	}
	else if(this->m_pIzicFile != NULL)
	{
		int totalCameraNum = this->m_pIzicFile->izicGetCameraCount();
		if(cameraNumber < totalCameraNum)
		{
			bret = true;
			m_bForceICNumberChange = true;
			m_ICNumber = cameraNumber;
		}
	}

	return bret;
}
