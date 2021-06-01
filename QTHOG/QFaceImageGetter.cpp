#include "QFaceImageGetter.h"
#include <Exception>
#include <QDebug>
#include "IplImageItem.h"
#include "RegionData.h"
#include "AviManager.h"

//QFaceImageGetter::QFaceImageGetter(QObject *parent, CvCapture *capture, QGraphicsItemGroup *pFaceItems, QGraphicsItemGroup *pPlateItems, StructData *pData)
//QFaceImageGetter::QFaceImageGetter(QObject *parent, AviManager *pAviManager, QGraphicsItemGroup *pFaceItems, QGraphicsItemGroup *pPlateItems, StructData *pData)
QFaceImageGetter::QFaceImageGetter(QObject *parent, std::map<SRegion::Type, QGraphicsItemGroup*> *pObjectItems, StructData *pData)
	: QThread(parent),
	m_stopped(false),
	m_pAviManager(0),
	m_pObjectItems(pObjectItems),
	m_pStructData(pData)//,
	//m_isRunning(false)
{
}

QFaceImageGetter::~QFaceImageGetter()
{
	//if (m_capture)
	//	cvReleaseCapture(&m_capture);
}
/*
int QFaceImageGetter::openAvi(const QString &filename)
{
	if (m_capture)
		cvReleaseCapture(&m_capture);

	m_capture = cvCaptureFromFile(filename.toLocal8Bit());

	m_frameCount = cvGetCaptureProperty(m_capture, CV_CAP_PROP_FRAME_COUNT);
	m_width = cvGetCaptureProperty(m_capture, CV_CAP_PROP_FRAME_WIDTH);
	m_height = cvGetCaptureProperty(m_capture, CV_CAP_PROP_FRAME_HEIGHT);

	return m_frameCount;
}
*/
/*
IplImage *AviManager::getImage(int frame)
{
	if (m_stillImage)
	{
		return m_stillImage;
	}

	if (!m_capture)
		return 0;

	if (frame < 0 || frame >= m_frameCount)
		return 0;

	cvSetCaptureProperty(m_capture, CV_CAP_PROP_POS_FRAMES, (double)frame);

	return cvQueryFrame(m_capture);
}
*/
/*
void QFaceImageGetter::showImage(int frame)
{
	if (!m_capture)
		return;

	if (frame < 0 || frame >= m_frameCount)
		return;

	cvSetCaptureProperty(m_capture, CV_CAP_PROP_POS_FRAMES, (double)frame);

	IplImage *image = cvQueryFrame(m_capture);
	cvNamedWindow("test");
	cvShowImage("test", image);
	cvWaitKey(10);
}
*/
static void convIplToQImage(cv::Mat image, QImage &dstImage, CvRect *rect)
{
	QRect r;
	if (rect)
		r = QRect(rect->x, rect->y, rect->width, rect->height);
	else
		r = QRect(0, 0, image.cols, image.rows);
	QImage tmp_image(reinterpret_cast<const uchar*>(image.data), image.cols, image.rows, image.step, QImage::Format_RGB888);
	dstImage = tmp_image.copy(r);

	//qDebug() << "kosshy chk convIplToQImage :" << r.x << " " << r.y << " " << r.width << " " << r.height;
	//qDebug() << "kosshy chk convIplToQImage :" << image->width << " " << image->height;

	//// 切り抜き元からはみだす矩形がきたら処理させない
	//if(r.x + r.width  >= image->width  || r.x < 0 ||
	//   r.y + r.height >= image->height || r.y < 0 )
	//{
	//	//qDebug() << "Setting image is out of range";
	//	return;
	//}

	//dstImage = QImage(r.width, r.height, QImage::Format_RGB888).copy();

	//CvMat mat;
	//for (int y = 0 ; y < r.height ; y++)
	//{
	//	cvInitMatHeader(&mat, 1, r.width, CV_8UC3, dstImage.scanLine(y));
	//	IplImage stub;
	//	IplImage *dst = cvGetImage(&mat, &stub);

	//	cvSetImageROI(image, cvRect(r.x, r.y + y, r.width, 1));
	//	cvCopy(image, dst);
	//}
	//cvResetImageROI(image);
}

void QFaceImageGetter::clearQue()
{
	m_objectImageQue.clear();
}

void QFaceImageGetter::run()
{
	//m_isRunning = true;
	int curFrame = -1;
	cv::Mat frameImage;

	while (!m_objectImageQue.empty())
	{
		// 対象フレームの画像を取得する。
		if (curFrame != m_objectImageQue.begin()->first)
		{
			m_pStructData->mutex.lock();
			frameImage = m_pAviManager->getImage(m_objectImageQue.begin()->first);
			m_pStructData->mutex.unlock();
			if (frameImage.empty())
			{
				curFrame = -1;
				continue;
			}
			curFrame = m_objectImageQue.begin()->first;
		}


		// フレーム内の再描画対象Item一覧
		std::vector<std::pair<SRegion::Type, int>> &items = m_objectImageQue.begin()->second;


		for (int i = 0; i < items.size(); ++i)
		{
			if (m_stopped)
				break;
			std::map<SRegion::Type, QGraphicsItemGroup*>::iterator itTargetItemGroup = m_pObjectItems->find(items[i].first);
			if (itTargetItemGroup == m_pObjectItems->end())
				continue;
			if (itTargetItemGroup->second->childItems().size() <= items[i].second)
				continue;
			IplImageItem *object = qgraphicsitem_cast<IplImageItem*>(itTargetItemGroup->second->childItems()[items[i].second]);

			StructData::iterator<SRegion> itr;
			itr = m_pStructData->regionFitAt(object->getTrackID());

			convIplToQImage(frameImage, object->m_image, &(itr->rect));

			object->setImageAvailable(true);
		}

		m_objectImageQue.pop_front();

		if (m_stopped)
		{
			m_stopped = false;
			m_objectImageQue.clear();
			return;
		}

		emit sgUpdateImage();
	}

}