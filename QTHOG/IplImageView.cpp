#include "IplImageView.h"
#include "IplScene.h"
#include <algorithm>
#include <QKeyEvent>
#include <QDebug>
#include <QScrollBar>
#include "RectItem.h"
#include <QApplication>
//#include <QWheelEvent>

IplImageView::IplImageView(QWidget *parent)
	: QGraphicsView(parent)//, m_image(0)
	, m_bRightDrag(false),
	m_iconOpacity(0)
{
	IplScene *scene = new IplScene(this);
	scene->setItemIndexMethod(QGraphicsScene::NoIndex);
	scene->setSceneRect(0, 0, 1000, 1000);
	setScene(scene);
	setCacheMode(QGraphicsView::CacheNone/*CacheBackground*/);
	setViewportUpdateMode(QGraphicsView::/*FullViewportUpdate*/BoundingRectViewportUpdate);
	setRenderHint(QPainter::Antialiasing);
	setTransformationAnchor(AnchorUnderMouse);
	setResizeAnchor(AnchorViewCenter);
	setDragMode(QGraphicsView::RubberBandDrag/*ScrollHandDrag*//*NoDrag*/);
	setOptimizationFlag(QGraphicsView::DontClipPainter, true);
	setBackgroundBrush(QBrush(QColor(70, 70, 70)));

	scene->setGraphicsView(this);

	connect(scene, SIGNAL(sgClicked(QGraphicsSceneMouseEvent *)), this, SIGNAL(sgClicked(QGraphicsSceneMouseEvent *)));
	connect(scene, SIGNAL(sgReleased(QGraphicsSceneMouseEvent *)), this, SIGNAL(sgReleased(QGraphicsSceneMouseEvent *)));

	connect(scene, SIGNAL(setScrollHandDrag()), this, SLOT(setModeScroll()));
	connect(scene, SIGNAL(setRubberBandDrag()), this, SLOT(setModeRubber()));

	connect(&m_timer, SIGNAL(timeout()), this, SLOT(onTimer()));

	QPixmap icon(":/Icon/QuestionIcon");
	m_questionIcon = icon;
	m_timer.setInterval(50);
}

void IplImageView::onTimer()
{
	updateView();
	m_iconOpacity -= 0.1;
	if (m_iconOpacity <= 0)
	{
		m_iconOpacity = 0;
		m_timer.stop();
	}
}

IplImageView::~IplImageView()
{
	//if (m_image)
	//	delete m_image;
}
/*
void IplImageView::mousePressEvent (QMouseEvent *event)
{
	event->ignore();
	//emit sgClicked(event);
}

void IplImageView::mouseReleaseEvent (QMouseEvent *event)
{
	event->ignore();
	//emit sgReleased(event);
}
*/
void IplImageView::setImage(cv::Mat image)
{
	//m_mutex.lock();
	m_image = QImage((uchar*)image.data, image.cols, image.rows, QImage::Format_RGB888).copy();
	//m_mutex.unlock();
/*
	if (m_image)
	{
		if (m_image->width() != image->width || m_image->height() != image->height)
		{
			delete m_image;
			m_image = new QImage((uchar*)image->imageData, image->width, image->height, QImage::Format_RGB888);
		}
	}
	else
	{
		m_image = new QImage((uchar*)image->imageData, image->width, image->height, QImage::Format_RGB888);
	}
*/
	//updateView();
}

void IplImageView::updateView()
{
	//m_mutex.lock();

	QList<QRectF> rects;
	QRectF visibleArea = mapToScene(viewport()->rect()/*rect()*/).boundingRect();

	rects.append(visibleArea);
	//QGraphicsView::updateSceneRect(visibleArea);
	QGraphicsView::updateScene(rects);

	//m_mutex.unlock();
}

void IplImageView::wheelEvent( QWheelEvent * event )
{
	emit sgWheelEvent(event);
}

void IplImageView::drawBackground(QPainter *painter, const QRectF &rect)
{
//    Q_UNUSED(rect);
	//m_mutex.lock();
	if (!m_image.isNull())
	{
		scene()->setSceneRect(0, 0, m_image.width(), m_image.height());
		painter->drawImage(0, 0, m_image.rgbSwapped());
	}
	//m_mutex.unlock();
}

void IplImageView::drawForeground(QPainter *painter, const QRectF &rect)
{
	QRectF visibleArea = mapToScene(viewport()->rect()/*this->rect()*/).boundingRect();
	QMatrix mat = matrix();
	
	float sz = 64 / mat.m11();

	QRectF target(visibleArea.center().x() - sz / 2, visibleArea.center().y() - sz / 2, sz, sz);
	QRectF source(0, 0, 64, 64);
	painter->setOpacity(m_iconOpacity);
	painter->drawPixmap(target, m_questionIcon, source);
	painter->setOpacity(1);
}

/*
void IplImageView::keyPressEvent ( QKeyEvent * e )
{
	if (e->modifiers() & Qt::ShiftModifier)
		setDragMode(QGraphicsView::ScrollHandDrag);

	e->accept();
}

void IplImageView::keyReleaseEvent ( QKeyEvent * e )
{
	if (e->key() == Qt::Key_Shift)
		setDragMode(QGraphicsView::RubberBandDrag);

	e->accept();
}
*/
float IplImageView::adjustScale(int w, int h)
{
	if (w < 0 || h < 0)
	{
		if (m_image.isNull())
			return 0;
		w = m_image.width();
		h = m_image.height();
	}

	QSize sz = size();
	float wr = (float)sz.width() / ((float)w * 1.05);
	float hr = (float)sz.height() / ((float)h * 1.05);
	float scale = wr < hr ? wr : hr;

	QMatrix scaleMat(scale, 0, 0, scale, 0, 0);
	setMatrix(scaleMat);

	return scale;
}

void IplImageView::mousePressEvent ( QMouseEvent * event )// {qDebug() << "pressed"; setDragMode(QGraphicsView::ScrollHandDrag); QGraphicsView::mousePressEvent(event);} 
{
	if (event->buttons() == Qt::RightButton)
	{
		QList<QGraphicsItem *> itms;
		itms = items(event->pos());
		QGraphicsItem* item;

		int faceCount = 0;
		foreach(item, itms)
		{
			RectItem *rect = (RectItem*)item;
			if (rect->getType() == SRegion::TypeFace)
				++faceCount;
		}
		if (faceCount > 0)
		{
			QGraphicsView::mousePressEvent(event);
			return;
		}

		QPoint rectCenter = rect().center();
		rectCenter.setX(rectCenter.x() - 8);
		rectCenter.setY(rectCenter.y() - 8);
		m_dragCenter = mapToScene(rectCenter);

		m_grabPoint = event->pos();
		m_bRightDrag = true;

		QCursor cursor;
		cursor.setShape(Qt::ClosedHandCursor);
		setCursor(cursor);
		IplScene *sc = (IplScene *)scene();
		sc->setMoveIgnore(true);
	}
	else
		QGraphicsView::mousePressEvent(event);
}

void IplImageView::mouseReleaseEvent ( QMouseEvent * event )
{
	if (m_bRightDrag)
	{
		m_bRightDrag = false;
		unsetCursor();
		IplScene *sc = (IplScene *)scene();
		sc->setMoveIgnore(false);
	}
	else
		QGraphicsView::mouseReleaseEvent(event);
}

void IplImageView::mouseMoveEvent ( QMouseEvent * event )
{
	if (m_bRightDrag)
	{
		QPointF newPos = event->pos();
		QMatrix mat = matrix();

		QPointF newCenter;// = m_dragCenter;
		newCenter.setX(m_dragCenter.x() + (m_grabPoint.x() - newPos.x()) / mat.m11());
		newCenter.setY(m_dragCenter.y() + (m_grabPoint.y() - newPos.y()) / mat.m11());

		QCursor cursor;
		cursor.setShape(Qt::ClosedHandCursor);
		setCursor(cursor);

		centerOn(newCenter);
	}
	else
		QGraphicsView::mouseMoveEvent(event);
}// {qDebug() << "moved";} 