#ifndef QFACEIMAGEGETTER_H
#define QFACEIMAGEGETTER_H

#include <QThread>
#include <opencv.hpp>
#include <QGraphicsView>
#include "RegionData.h"


class StructData;
class AviManager;

class QFaceImageGetter : public QThread
{
	Q_OBJECT

public:
	//QFaceImageGetter(QObject *parent, CvCapture *capture, QGraphicsItemGroup *faceItems, QGraphicsItemGroup *plateItems, StructData *pData);
	//QFaceImageGetter(QObject *parent, AviManager *pAviManager, QGraphicsItemGroup *faceItems, QGraphicsItemGroup *plateItems, StructData *pData);
	QFaceImageGetter(QObject *parent, std::map<SRegion::Type, QGraphicsItemGroup*> *pObjectItems, StructData *pData);
	~QFaceImageGetter();

private:
	//CvCapture *m_capture;
	AviManager *m_pAviManager;
	int m_frameCount;
	int m_width;
	int m_height;	
	StructData *m_pStructData;
	//QGraphicsItemGroup *m_pFaceItems, *m_pPlateItems;
	std::map<SRegion::Type, QGraphicsItemGroup*> *m_pObjectItems;
	std::deque<std::pair<int, std::vector<std::pair<SRegion::Type, int>>>> m_objectImageQue;
	volatile bool m_stopped;
	//volatile bool m_isRunning;

public:
	//int openAvi(const QString &filename);

	//void setStructData(StructData *data) {m_pStructData = data;}
	//void setImageItems(QGraphicsItemGroup *items) {m_pImageItems = items;}

	//void showImage(int frame);
	void addObjectImageQue(int frame, std::vector<std::pair<SRegion::Type, int>> &items) {m_objectImageQue.push_back(std::make_pair(frame, items));}
	void stop() {m_stopped = true;}
	//bool isRun() {return m_isRunning;}
	void clearQue();
	int getObjectQueCount() {return m_objectImageQue.size();}
	void setStructData(StructData *pData) {m_pStructData = pData;}
	void setAviManager(AviManager *pAviManager) {m_pAviManager = pAviManager;}

protected:
	void run();

signals:
	void sgUpdateImage();
};


#endif // QFACEIMAGEGETTER_H
