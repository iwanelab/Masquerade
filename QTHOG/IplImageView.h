#ifndef IPLIMAGEVIEW_H
#define IPLIMAGEVIEW_H

#include <QGraphicsView>
#include <opencv.hpp>
#include <QDebug>
#include <QTimer>
#include <QMutex>
//#include "structdata.h"

struct SRectDrawing
{
	CvRect rect;
	unsigned char r, g, b;
};

class IplImageView : public QGraphicsView
{
Q_OBJECT
private:
	//int m_frame;

	bool m_bRightDrag;
	QPointF m_dragCenter;
	QPointF m_grabPoint;
	QPixmap m_questionIcon;
	QTimer m_timer;
	float m_iconOpacity;
	
public:
	QImage m_image;


protected:
	virtual void drawBackground(QPainter *painter, const QRectF &rect);
	virtual void drawForeground(QPainter *painter, const QRectF &rect);
	//void IplImageView::keyPressEvent ( QKeyEvent * e );
	//void IplImageView::keyReleaseEvent ( QKeyEvent * e );
	void mousePressEvent ( QMouseEvent * event );// {qDebug() << "pressed"; setDragMode(QGraphicsView::ScrollHandDrag); QGraphicsView::mousePressEvent(event);} 
	void mouseReleaseEvent ( QMouseEvent * event ); 
	void mouseMoveEvent ( QMouseEvent * event );// {qDebug() << "moved";} 
	void wheelEvent ( QWheelEvent * event );

public:
	IplImageView(QWidget *parent = 0);
	~IplImageView();

	void setImage(cv::Mat image);
	float adjustScale(int w = -1, int h = -1);
	void updateView();
	void showIcon() {m_iconOpacity = 1; m_timer.start();}

public slots:
	void setModeScroll() {setDragMode(QGraphicsView::ScrollHandDrag);}
	void setModeRubber() {setDragMode(QGraphicsView::RubberBandDrag);}
	void onTimer();


signals:
	void sgClicked(QGraphicsSceneMouseEvent *mouseEvent);
	void sgReleased(QGraphicsSceneMouseEvent *mouseEvent);

	void sgWheelEvent(QWheelEvent * event);

	//void sgClicked(QMouseEvent *mouseEvent);
	//void sgReleased(QMouseEvent *mouseEvent);
};

#endif // IPLIMAGEVIEW_H
