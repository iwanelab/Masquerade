#ifndef MERCATORVIEW_H
#define MERCATORVIEW_H

#include "IplImageView.h"
#include "IplScene.h"
#include <set>
#include "Constants.h"
#include "RegionData.h"
//class StructData;

class MercatorView : public IplImageView
{
	Q_OBJECT

public:
	MercatorView(QWidget *parent);
	~MercatorView();

private:
	StructData *m_pStructData;
	std::vector<SRectDrawing> m_rects;
	QWidget *m_pParentDialog;

	std::set<int> *m_pSelectedItems;
	int *m_pLastSelected;
	std::vector<SRegion::Type> *m_pTargetType;
	int m_curFrame;
	int m_curCamera;
	//QGraphicsItemGroup *m_pFaces, *m_pPlates, *m_pHOGs;
	std::map<SRegion::Type, QGraphicsItemGroup*> m_pItemGroup;

	// ï`âÊRegionêî
	int m_numViewRegion;

protected:
	void drawBackground(QPainter *painter, const QRectF &rect);
	void keyPressEvent(QKeyEvent *event);

public:
	void setStructData(StructData *pStructData) {pStructData->retain(); m_pStructData = pStructData;}
	void setFrame(int frame = -1);
	void setParentDialog(QWidget *pWidget) {m_pParentDialog = pWidget;}

	void setSelectedItems(std::set<int> *pSelectedItems) {m_pSelectedItems = pSelectedItems;}
	void setLastSelected(int *pSelected) {m_pLastSelected = pSelected;}
	void setTargetType(std::vector<SRegion::Type> *ptargetType) {m_pTargetType = ptargetType;}

	// ÉJÉÅÉâêÿÇËë÷Ç¶ é∏îsÇµÇΩÇÁfalse
	bool changeCamera(int cameraNumber);

	// ï`âÊRegionêî
	int getNumViewRegion() { return m_numViewRegion; }

public slots:
	void onItemMoved(int id, QPointF pos);
	void onItemResized(int id, QRectF newRect);

signals:
	void sgItemChanged();
	void sgItemDeleted(const std::set<int> &deleteSet);
	void sgImageChanged(const std::vector<int> &changedImages, bool bSelect);
};

#endif // MERCATORVIEW_H
