#ifndef FACEVIEW_H
#define FACEVIEW_H

#include <QGraphicsView>
#include <QResizeEvent>
#include <opencv.hpp>
#include <set>
#include "RegionData.h"

//class StructData;
class AviManager;
class DlgFaces;

class FaceView : public QGraphicsView
{
	Q_OBJECT

public:
	enum {FaceMargin = 10/*20*/, FaceDispSize = 32};

	FaceView(QWidget *parent = 0);
	~FaceView();

protected:
	StructData *m_pStructData;
	AviManager *m_pAviManager;
	int m_frame;
	int m_camera;
	float m_scale;
	std::set<int> *m_pSelectedItems;
	DlgFaces *m_pParentDialog;
	int *m_pLastSelected;

protected:
	bool isFrameValid(int f);
	void convIplToQImage(cv::Mat image, QImage &dstImage, CvRect *rect = 0);

public:
	void setStructData(StructData *pStructData) {pStructData->retain(); m_pStructData = pStructData;}
	void setAviManager(AviManager *pAviManager) {m_pAviManager = pAviManager;}
	void setSelectedItems(std::set<int> *pSelectedItems) {m_pSelectedItems = pSelectedItems;}
	void setLastSelected(int *pSelected) {m_pLastSelected = pSelected;}
	void setParentDialog(DlgFaces *parent) {m_pParentDialog = parent;}

	void setFrame(int f);
	virtual void updateView() = 0;
	virtual void setScale(float scale) {m_scale = scale; /*alignFaces();*/}
	float adjustScale(int w = -1, int h = -1);

	// ÉJÉÅÉâêÿÇËë÷Ç¶ é∏îsÇµÇΩÇÁfalse
	bool changeCamera(int cameraNumber);
};


#endif // FACEVIEW_H
