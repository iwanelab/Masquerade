#ifndef IPLSCENE_H
#define IPLSCENE_H

#include <QGraphicsScene>
#include <opencv.hpp>

class RectItem;
class StructData;

class IplScene : public QGraphicsScene
{
Q_OBJECT

private:
	enum eDragMode {NONE, MOVE, HR1, HR2, VR1, VR2, BD1, BD2, FD1, FD2, HAND_SCROLL} m_dragMode;
	bool m_bDragging;
	QPointF m_grabPoint;
	RectItem *m_targetItem;
	QGraphicsView *m_graphicsView;
	bool m_bMoveIgnore;

public:
	IplScene(QObject *parent = 0);

protected:
	void mousePressEvent (QGraphicsSceneMouseEvent *mouseEvent);
	void mouseReleaseEvent (QGraphicsSceneMouseEvent *mouseEvent);
	void mouseMoveEvent ( QGraphicsSceneMouseEvent * mouseEvent );

signals:
	void sgClicked(QGraphicsSceneMouseEvent *mouseEvent);
	void sgReleased(QGraphicsSceneMouseEvent *mouseEvent);

	void setScrollHandDrag();
	void setRubberBandDrag();

	void sgItemMoved(int id, QPointF pos);
	void sgItemResized(int id, QRectF newRect);

public:
	void setGraphicsView(QGraphicsView *view) {m_graphicsView = view;}
	void setMoveIgnore(bool bIgnore) {m_bMoveIgnore = bIgnore;}
};

#endif // IPLSCENE_H
