#ifndef IPLIMAGEITEM_H
#define IPLIMAGEITEM_H

#include "CustomGraphicsItem.h"
//#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>

class IplImageItem : public CustomGraphicsItem
{
	Q_OBJECT

public:
	IplImageItem(QGraphicsItem *parent = 0);
	~IplImageItem();

	// ‘ã“ü‰‰ŽZŽq
	IplImageItem &IplImageItem::operator=(const IplImageItem &rh)
	{
		m_frameColor = rh.m_frameColor;
		m_trackID = rh.m_trackID;
		m_frameID = rh.m_frameID;
		m_objectID = rh.m_objectID;
		m_type = rh.m_type;
		m_bSelected = false;
		m_bCenterOnClicked = false;
		m_image = rh.m_image.copy();

		return *this;
	}

	int type() const {return CustomGraphicsItem::Image;}

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget);

public:
	enum eTypes {IMAGE, RECT, LARROW, RARROW};

private:
	QColor m_frameColor;
	int m_trackID;
	int m_frameID;
	int m_objectID;
	eTypes m_type;
	bool m_bSelected;
	bool m_bCenterOnClicked;

	bool m_bImageAvailable;

	enum {MarkerSize = 12};

public:
	QImage m_image;

protected:
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
	void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);

signals:
	//void sgItemClicked(int, bool, Qt::MouseButtons);
	void sgItemClicked(int, int, int, bool, Qt::MouseButtons);
	void sgItemReleased(int, int, int, bool, Qt::MouseButtons);
	void sgItemDoubleClicked(int);
	void sgItemDoubleClicked(int, int, bool);

public:
	void setFrameColor(QColor color) {m_frameColor = color;}
	void setTrackIndices(int track, int frame, int object) {m_trackID = track; m_frameID = frame; m_objectID = object;}

	void setTrackID(int track) {m_trackID = track;}
	void setObjectID(int object) {m_objectID = object;}
	void setframeID(int frame) {m_frameID = frame;}

	int getTrackID() {return m_trackID;}
	int getObjectID() {return m_objectID;}
	int getFrameID() {return m_frameID;}
	void setSelected(bool bSelected) {m_bSelected = bSelected;}
	bool isSelected() {return m_bSelected;}
	void setType(eTypes typ) {m_type = typ;}
	eTypes getType() {return m_type;}
	void setCenterOnClicked(bool bCenterOn) {m_bCenterOnClicked = bCenterOn;}
	void setImageAvailable(bool bAvailable) {m_bImageAvailable = bAvailable;}
	bool checkImageAvailable() {return m_bImageAvailable;}
	void clearImage();
};

#endif // IPLIMAGEITEM_H
