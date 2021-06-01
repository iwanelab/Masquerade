#include "IplScene.h"
#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include <QList>
#include <QCursor>
#include <QGraphicsView>
#include "RectItem.h"

IplScene::IplScene(QObject *parent)
	: QGraphicsScene(parent),
	m_dragMode(NONE),
	m_targetItem(0),
	m_bDragging(false),
	m_bMoveIgnore(false)
{
}

void IplScene::mousePressEvent (QGraphicsSceneMouseEvent *mouseEvent)
{
	if (!m_bDragging && m_dragMode != NONE)
	{
		m_bDragging = true;
		mouseEvent->accept();
		qDebug() << "press accept";
		return;
	}

	emit(sgClicked(mouseEvent));
}

void IplScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
	if (m_bDragging && m_dragMode != NONE)
	{
		//m_dragMode = NONE;
		m_bDragging = false;
		//m_targetItem = 0;
		mouseEvent->accept();
		qDebug() << "release accept";

		QPointF mousePos = mouseEvent->lastScenePos();
		switch (m_dragMode)
		{
		case MOVE:
			{
				//if ((m_targetItem->pos() - mousePos + m_grabPoint).manhattanLength() < 5)
				//	emit sgReleased(mouseEvent);
				//else
					emit sgItemMoved(m_targetItem->getTrackID(), mousePos - m_grabPoint);
				break;
			}
		case HR1: case HR2: case VR1: case VR2: case BD1: case BD2: case FD1: case FD2:
			{
				QRectF newRect(m_targetItem->pos(), QSizeF(m_targetItem->getWidth(), m_targetItem->getHeight()));
				emit sgItemResized(m_targetItem->getTrackID(), newRect); 
				break;
			}
		}

		return;
	}
	emit sgReleased(mouseEvent);
}

void IplScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
	if (m_bMoveIgnore)
		return;

	if (!m_bDragging)
	{
		m_dragMode = NONE;

		QList<QGraphicsItem*> items = this->items(mouseEvent->lastScenePos());
		QGraphicsItem *item;
		m_targetItem = 0;
		QCursor cursor;
		
		foreach(item, items)
		{
			RectItem *rect = (RectItem*)item;
			if (rect->getType() != SRegion::TypeFace && rect->getType() != SRegion::TypePlate)
				continue;

			if (!rect->bSelected())
				continue;

			QPointF itemPoint = rect->mapFromScene(mouseEvent->lastScenePos());
			QRectF bounding = rect->boundingRect();
			if (bounding.contains(itemPoint))
			{
				if (itemPoint.x() < 1.5)
				{
					if (itemPoint.y() < 1.5)
					{
						cursor.setShape(Qt::SizeFDiagCursor);
						m_dragMode = FD1;
					}
					else if (itemPoint.y() > bounding.height() - 1.5)
					{
						cursor.setShape(Qt::SizeBDiagCursor);
						m_dragMode = BD2;
					}
					else
					{
						cursor.setShape(Qt::SizeHorCursor);
						m_dragMode = HR1;
					}
				}
				else if (itemPoint.x() > bounding.width() - 1.5)
				{
					if (itemPoint.y() < 1.5)
					{
						cursor.setShape(Qt::SizeBDiagCursor);
						m_dragMode = BD1;
					}
					else if (itemPoint.y() > bounding.height() - 1.5)
					{
						cursor.setShape(Qt::SizeFDiagCursor);
						m_dragMode = FD2;
					}
					else
					{
						cursor.setShape(Qt::SizeHorCursor);
						m_dragMode = HR2;
					}
				}
				else if (itemPoint.y() > bounding.height() - 1.5)
				{
					cursor.setShape(Qt::SizeVerCursor);
					m_dragMode = VR2;
				}
				else if (itemPoint.y() < 1.5)
				{
					cursor.setShape(Qt::SizeVerCursor);
					m_dragMode = VR1;
				}
				else
				{
					cursor.setShape(Qt::SizeAllCursor);
					m_dragMode = MOVE;
					m_grabPoint = itemPoint;
				}
					
				m_targetItem = rect;
				break;
			}	// if (bounding.contains(itemPoint))
		}	// foreach(item, items)

		if (!m_targetItem)
			m_graphicsView->unsetCursor();
		else
			m_graphicsView->setCursor(cursor);
	}
	else
	{
		switch (m_dragMode)
		{
		case MOVE:
			{
				m_targetItem->setPos(mouseEvent->lastScenePos() - m_grabPoint);
				update(0, 0, width(), height());
				break;
			}
		case HR1:
			{
				float oldWidth = m_targetItem->getWidth();
				QPointF tl = m_targetItem->pos();
				float newWidth = tl.x() + oldWidth - mouseEvent->lastScenePos().x();
				if (newWidth < 5)
					break;

				m_targetItem->setPos(mouseEvent->lastScenePos().x(), tl.y());
				m_targetItem->setSize(newWidth, m_targetItem->getHeight());

				update(0, 0, width(), height());
				break;
			}
		case HR2:
			{
				QPointF tl = m_targetItem->pos();
				float newWidth = mouseEvent->lastScenePos().x() - tl.x();
				if (newWidth < 5)
					break;

				m_targetItem->setSize(newWidth, m_targetItem->getHeight());

				update(0, 0, width(), height());
				break;
			}
		case VR1:
			{
				float oldHeight = m_targetItem->getHeight();
				QPointF tl = m_targetItem->pos();
				float newHeight = tl.y() + oldHeight - mouseEvent->lastScenePos().y();
				if (newHeight < 5)
					break;

				m_targetItem->setPos(tl.x(), mouseEvent->lastScenePos().y());
				m_targetItem->setSize(m_targetItem->getWidth(), newHeight);

				update(0, 0, width(), height());
				break;
			}
		case VR2:
			{
				QPointF tl = m_targetItem->pos();
				float newHeight = mouseEvent->lastScenePos().y() - tl.y();
				if (newHeight < 5)
					break;

				m_targetItem->setSize(m_targetItem->getWidth(), newHeight);

				update(0, 0, width(), height());
				break;
			}
		case BD1:
			{
				float oldHeight = m_targetItem->getHeight();
				QPointF tl = m_targetItem->pos();
				float newHeight = tl.y() + oldHeight - mouseEvent->lastScenePos().y();
				float newWidth = mouseEvent->lastScenePos().x() - tl.x();
				
				if (newWidth < 5)
					break;
				if (newHeight < 5)
					break;

				m_targetItem->setPos(tl.x(), mouseEvent->lastScenePos().y());
				m_targetItem->setSize(newWidth, newHeight);

				update(0, 0, width(), height());
				break;
			}
		case BD2:
			{
				float oldWidth = m_targetItem->getWidth();
				QPointF tl = m_targetItem->pos();
				float newWidth = tl.x() + oldWidth - mouseEvent->lastScenePos().x();
				float newHeight = mouseEvent->lastScenePos().y() - tl.y();
				
				if (newWidth < 5)
					break;
				if (newHeight < 5)
					break;

				m_targetItem->setPos(mouseEvent->lastScenePos().x(), tl.y());
				m_targetItem->setSize(newWidth, newHeight);

				update(0, 0, width(), height());
				break;
			}
		case FD1:
			{
				float oldWidth = m_targetItem->getWidth();
				float oldHeight = m_targetItem->getHeight();
				QPointF tl = m_targetItem->pos();
				float newWidth = tl.x() + oldWidth - mouseEvent->lastScenePos().x();
				float newHeight = tl.y() + oldHeight - mouseEvent->lastScenePos().y();
				
				if (newWidth < 5)
					break;
				if (newHeight < 5)
					break;

				m_targetItem->setPos(mouseEvent->lastScenePos().x(), mouseEvent->lastScenePos().y());
				m_targetItem->setSize(newWidth, newHeight);

				update(0, 0, width(), height());
				break;
			}
		case FD2:
			{
				QPointF tl = m_targetItem->pos();
				float newWidth = mouseEvent->lastScenePos().x() - tl.x();
				float newHeight = mouseEvent->lastScenePos().y() - tl.y();
				
				if (newWidth < 5)
					break;
				if (newHeight < 5)
					break;

				m_targetItem->setSize(newWidth, newHeight);

				update(0, 0, width(), height());
				break;
			}
		}
	}
}