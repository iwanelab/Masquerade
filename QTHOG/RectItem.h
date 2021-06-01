#ifndef RECTITEM_H
#define RECTITEM_H

//#include <QGraphicsItem>
#include "CustomGraphicsItem.h"
#include <QGraphicsSceneMouseEvent>
#include "RegionData.h"

//class RectItem : public QObject, public QGraphicsItem
class RectItem : public CustomGraphicsItem
{
	Q_OBJECT

public:
	RectItem(QGraphicsItem *parent = 0);
	~RectItem();

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget);

	int type() const {return CustomGraphicsItem::Rect;}

private:
	QColor m_frameColor;
	int m_trackID;
	int m_frameID;
	int m_objectID;
	SRegion::Type m_type;
	float m_width;
	float m_height;
	bool m_bSelected;

protected:
	void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);

signals:
	//void sgItemClicked(int, bool);
	void sgItemDoubleClicked(int);

public:
	void setFrameColor(QColor color) {m_frameColor = color;}
	void setTrackIndices(int track, int frame, int object, SRegion::Type type)
					{m_trackID = track; m_frameID = frame; m_objectID = object; m_type = type;}
	int getTrackID() {return m_trackID;}
	int getObjectID() {return m_objectID;}
	int getFrameID() {return m_frameID;}
	SRegion::Type getType() {return m_type;}
	void setSize(float w, float h) {m_width = w; m_height = h;}
	float getWidth() {return m_width;}
	float getHeight() {return m_height;}
	void setSelected(bool bSelected) {m_bSelected = bSelected;}
	bool bSelected() {return m_bSelected;}
};

#endif // RECTITEM_H
