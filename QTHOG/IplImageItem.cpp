#include "IplImageItem.h"
#include <QPainter>
#include <QDebug>

IplImageItem::IplImageItem(QGraphicsItem *parent) :
	CustomGraphicsItem(parent),
	m_frameColor(QColor(200, 200, 200)),
	m_trackID(-1),
	m_bSelected(false),
	m_bCenterOnClicked(true),
	m_bImageAvailable(false)
{
	m_image = QImage(16, 16, QImage::Format_RGB888);
	m_image.fill(0x808080);

	//setFlags(ItemIsSelectable | ItemIsMovable);
}

IplImageItem::~IplImageItem()
{
	//if (m_pImage)
	//	delete m_pImage;
}

QRectF IplImageItem::boundingRect() const
{
	qreal adjust = 0.5;

	switch (m_type)
	{
	case IMAGE:
		{
			qreal halfW = m_image.width() / 2;
			qreal halfH = m_image.height() / 2;

			return QRectF(-adjust - halfW, -adjust - halfH, 2 * halfW + adjust, 2 * halfH + adjust);
		}
	case RARROW:
	case LARROW:
	case RECT:
		{
			return QRectF(-adjust - MarkerSize, -adjust - MarkerSize, 2 * MarkerSize + adjust, 2 * MarkerSize + adjust);
		}
	}
	return QRectF(0, 0, 0, 0);
}
/*
QPainterPath IplImageItem::shape() const
{
    QPainterPath path;
    path.addRect(-10, -20, 20, 40);

    return path;
}
*/
void IplImageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
	switch (m_type)
	{
	case IMAGE:
		{
			qreal halfW = m_image.width() / 2;
			qreal halfH = m_image.height() / 2;

			painter->drawImage(QPointF(-halfW, -halfH), m_image.rgbSwapped());

			if (m_bSelected)
				//painter->fillRect(QRectF(-halfW - 2, -halfH - 2, 2 * halfW + 4, 2 * halfH + 4), QColor(160, 160, 160));
				painter->fillRect(QRectF(-halfW - 2, -halfH - 2, 2 * halfW + 4, 2 * halfH + 4),
									QColor(m_frameColor.red(), m_frameColor.green(), m_frameColor.blue(), 80));

			QPen pen;
			pen.setColor(m_frameColor);
			if (m_bSelected)
				pen.setWidthF(1.5);
			painter->setPen(pen);
			painter->drawRect(QRectF(-halfW, -halfH, 2 * halfW, 2 * halfH));
		}
		break;
	case RARROW:
	case LARROW:
	case RECT:
		{
			painter->fillRect(QRectF(-MarkerSize, -MarkerSize, 2 * MarkerSize, 2 * MarkerSize), m_frameColor);
		}
		break;
	}
}

void IplImageItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
/*
    if (event->button() != Qt::LeftButton)
	{
        event->ignore();
        return;
    }
*/
	//if (m_trackID >= 0)
		//emit sgItemClicked(m_trackID, event->modifiers() & Qt::ControlModifier, m_bCenterOnClicked, event->button());
		emit sgItemClicked(m_trackID, m_frameID, m_objectID, event->modifiers() & Qt::ControlModifier, event->button());
}

void IplImageItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	//emit sgItemReleased(m_trackID, m_frameID, m_objectID, event->modifiers() & Qt::ControlModifier, event->button());
}

void IplImageItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_trackID >= 0)
		emit sgItemDoubleClicked(m_trackID);
	else
		emit sgItemDoubleClicked(m_objectID, m_frameID, event->modifiers() & Qt::ControlModifier);
}

void IplImageItem::clearImage()
{
	m_image.fill(0x808080);
}