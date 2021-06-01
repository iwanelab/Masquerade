#ifndef MERGEVIEW_H
#define MERGEVIEW_H

#include "FaceView.h"
#include <QTimer>

class IplImageItem;

class MergeView : public FaceView
{
	Q_OBJECT

public:
	MergeView(QWidget *parent);
	~MergeView();

	void updateView();
	void setScale(float scale);
	void addImageItem(IplImageItem *item);
	void clearImageItem() {scene()->clear();}
	void getMapTrackToObject(std::map<int, int>& t2o);
	void pack();
	void setStructData(StructData *pStructData) {pStructData->retain(); m_pStructData = pStructData;}

protected:
	virtual void drawBackground(QPainter *painter, const QRectF &rect);
	virtual void showEvent(QShowEvent *event);
	virtual void mouseMoveEvent(QMouseEvent *event);
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
	virtual void keyPressEvent(QKeyEvent *event);

private:
	enum eDragMode {NONE, MOVE} m_dragMode;
	IplImageItem *m_targetItem;
	StructData *m_pStructData;
	float m_scale;
	bool m_bMidDrag;
	bool m_bRightDrag;
	bool m_bDragging;

	QPointF m_selectedRegion[2];
	std::vector<int> m_gridX;
	std::vector<int> m_gridY;
	std::vector<int> m_gridWidth;
	std::vector<int> m_gridHeight;
	int m_firstFrame;
	int m_lastFrame;
	std::map<int, int> m_mapObjectsToIndex;
	std::map<int, int> m_mapIndexToObjects;

	std::set<IplImageItem*> m_selectedItems;
	int m_highLightRow;
	std::set<int> m_highLightCols;

	QPointF m_dragCenter;
	QPointF m_grabPoint;
	QTimer m_timer;

signals:
	void sgScaleChanged(float scale);

private:
	void calcGrid();
	void selectFaces(bool bCtrl);
	bool checkObjectType(int targetObjectId, int sourceObjectId);
};

#endif // MERGEVIEW_H
