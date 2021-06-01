#include "FaceView.h"
/*
FaceView::FaceView(QWidget *parent)
	: QGraphicsView(parent)
{

}

FaceView::~FaceView()
{

}
*/
#include "FrameFaceScene.h"
#include "RegionData.h"
#include "AviManager.h"
#include "IplImageItem.h"
#include "RegionData.h"

FaceView::FaceView(QWidget *parent)
	: QGraphicsView(parent),
	m_pAviManager(0),
	m_pStructData(0),
	m_frame(-1),
	m_camera(0),
	m_scale(1.0f)
{
	QGraphicsScene *scene = new FrameFaceScene(this);
	scene->setItemIndexMethod(QGraphicsScene::NoIndex);
	scene->setSceneRect(0, 0, 1000, 1000);
	setScene(scene);
	setBackgroundBrush(QBrush(QColor(70, 70, 70)));
}

FaceView::~FaceView()
{
	if (m_pStructData)
		m_pStructData->release();
}

bool FaceView::isFrameValid(int f)
{
	if (!m_pAviManager || !m_pStructData)
		return false;
	if (f < 0 || f >= m_pAviManager->getFrameCount())
		return false;

	return true;
}

void FaceView::setFrame(int f)
{
	if (!isFrameValid(f))
		return;

	m_frame = f;
	updateView();
}

void FaceView::convIplToQImage(cv::Mat image, QImage &dstImage, CvRect *rect)
{
	QRect r;
	if (rect)
		r = QRect(rect->x, rect->y, rect->width, rect->height);
	else
		r = QRect(0, 0, image.cols, image.rows);
	QImage tmp_image(reinterpret_cast<const uchar*>(image.data), image.cols, image.rows, image.step, QImage::Format_RGB888);
	dstImage = tmp_image.copy(r);
	//CvRect r;
	//if (rect)
	//{
	//	r = *rect;
	//}
	//else
	//{
	//	r.x = 0;
	//	r.y = 0;
	//	r.width = image->width;
	//	r.height = image->height;
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

float FaceView::adjustScale(int w, int h)
{
	if (w < 0 || h < 0)
	{
		w = sceneRect().width();
		h = sceneRect().height();
	}

	QSize sz = size();
	float wr = (float)sz.width() / ((float)w * 1.05);
	float hr = (float)sz.height() / ((float)h * 1.05);
	float scale = wr < hr ? wr : hr;

	//QMatrix scaleMat(scale, 0, 0, scale, 0, 0);
	//setMatrix(scaleMat);

	return scale;
}

// ƒJƒƒ‰Ø‚è‘Ö‚¦ Ž¸”s‚µ‚½‚çfalse
bool FaceView::changeCamera(int cameraNumber)
{
	bool bret = true;

	this->m_camera = cameraNumber;

	return bret;
}
