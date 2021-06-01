#ifndef FACEGRIDVIEW_H
#define FACEGRIDVIEW_H

#include "windows.h"
#include "FaceView.h"
#include "IplImageItem.h"
#include "QFaceImageGetter.h"
#include <opencv.hpp>
#include "Constants.h"

class DlgFaceGrid;
class AviManager;

struct UpdateInfo
{
	enum UpdateType {SelectionChanged, ImageChanged} type;
	std::vector<int> changedImages;
	bool bSelect;
	//int workFrame;
};

class FaceGridView : public FaceView
{
	Q_OBJECT

public:
	FaceGridView(QWidget *parent = 0);
	~FaceGridView();
	void updateView() {updateView(true);}
	void updateView(bool bCenterOn, UpdateInfo *info = 0);
	void setScale(float scale);
	void setParentDialog(DlgFaceGrid *parent) {m_pParentDialog = parent;}
	void setFrame(int frame, bool bUpdate = false);
	void setIgnoreFrame(bool bIgnore) {m_bIgnoreFrame = bIgnore;}
	void setStructData(StructData *data) {FaceView::setStructData(data); m_pFaceImageGetter->setStructData(data);}
	void openAvi(QString filename);
	void eraseItems(const std::set<int> &eraseList);
	void exportSelectedImages(const QString &filename);
	void exportSamples(const QString &dirname, bool bPositive);
	void exportSamplesHeadShoulder(const QString &dirname, bool bPositive);
	void exportSamplesHeadShoulderNegative(const QString &dirname);
	void stopImageGetter();
	void setWaitRegion();
	void unsetWaitRegion();
	void setAviManager(AviManager *pAviManager) {m_pAviManager = pAviManager;m_pFaceImageGetter->setAviManager(m_pAviManager);}
	void setTargetType(std::vector<SRegion::Type> *ptargetType) {m_pTargetType = ptargetType;}

protected:
	virtual void drawBackground(QPainter *painter, const QRectF &rect);
	virtual void mouseMoveEvent(QMouseEvent *event);
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
	virtual void wheelEvent(QWheelEvent *event);
	virtual void mouseDoubleClickEvent(QMouseEvent *event);

	virtual void keyPressEvent (QKeyEvent *event);

private:
	float m_scale;
	std::vector<int> m_gridX;
	std::vector<int> m_gridY;
	std::vector<int> m_gridWidth;
	std::vector<int> m_gridHeight;
	//std::map<int, int> m_mapFaceItems, m_mapPlateItems;
	std::map<SRegion::Type, std::map<int, int>> m_mapObjectItems;
	bool m_bIgnoreFrame;

	int m_firstFrame;
	int m_lastFrame;
	DlgFaceGrid *m_pParentDialog;
	int m_curFrame;
	//int m_curCamera;
	std::set<int> m_curObjects;
	bool m_bInDoubleClicking;

	enum eDragMode {NONE, MOVE} m_dragMode;
	bool m_bMidDrag;
	bool m_bRightDrag;
	QPointF m_dragCenter;
	QPointF m_grabPoint;
	QPointF m_selectedRegion[2];

	QFaceImageGetter *m_pFaceImageGetter;
	//CvCapture *m_pCapture;
	//AviManager *m_pAviManager;
	//Constants::DetectType m_detectType;
	bool m_bWaitRegion;
	std::vector<SRegion::Type> *m_pTargetType;

public:
	//QGraphicsItemGroup *m_pFaceItems, *m_pArrowItems, *m_pPlateItems;
	std::map<SRegion::Type, QGraphicsItemGroup*> m_pObjectItems;
	QGraphicsItemGroup *m_pArrowItems;

private:
	void calcGrid(int frame);
	void selectFaces(Qt::KeyboardModifiers modifiers);//bool bCtrl);
	IplImageItem* getViewObject(QGraphicsItemGroup *pGraphicsItems, StructData::iterator<SRegion>& region);
	void addImageAcquisitionList(std::map<int, std::vector<std::pair<SRegion::Type, int>>> &forwardItemList, std::map<int, std::vector<std::pair<SRegion::Type, int>>> &backwardItemList, int objectId, IplImageItem* object, StructData::iterator<SRegion>& region);

signals:
	void sgScaleChanged(float scale);
	void sgSelectionChanged();
	void sgWheelEvent(QWheelEvent *event);
	void sgMoveUp();
	void sgMoveDown();
	void sgMoveRight();
	void sgMoveLeft();
	void sgEnlarge();
	void sgShrink();

private slots:
	void onUpdateImage();

};

#endif // FACEGRIDVIEW_H
