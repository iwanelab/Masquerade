#include "RectItem.h"

#include <QPainter>
#include <QDebug>
#include <QCursor>

RectItem::RectItem(QGraphicsItem *parent) :
	CustomGraphicsItem(parent),
	m_frameColor(QColor(200, 200, 200)),
	m_trackID(-1),
	m_frameID(-1),
	m_objectID(-1),
	m_width(10),
	m_height(10),
	m_bSelected(false)//,
	//m_dragMode(NONE)
{
	//viewport()->setMouseTracking(true);
	setAcceptHoverEvents(true);
	//setFlags(ItemIsSelectable | ItemIsMovable);
}

RectItem::~RectItem()
{

}

QRectF RectItem::boundingRect() const
{
	qreal adjust = 0.5;

	return QRectF(0 - adjust, 0 - adjust, m_width + adjust, m_height + adjust);
}

void RectItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
	if (m_bSelected)
		painter->fillRect(QRectF(- 2, - 2, m_width + 4, m_height + 4),
							QColor(m_frameColor.red(), m_frameColor.green(), m_frameColor.blue(), 80));

	//painter->drawImage(QPointF(-halfW, -halfH), m_image.rgbSwapped());

	QPen pen;
	pen.setColor(m_frameColor);

	if (m_bSelected)
		pen.setWidthF(1.5);
	painter->setPen(pen);
	painter->drawRect(QRectF(0, 0, m_width, m_height));
}

void RectItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	emit sgItemDoubleClicked(m_trackID);
}
