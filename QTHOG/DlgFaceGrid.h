#ifndef DLGFACEGRID_H
#define DLGFACEGRID_H

#include <QMainWindow>
#include "ui_DlgFaceGrid.h"
#include "DlgMerge.h"
#include <QMouseEvent>
#include <set>
#include "Constants.h"

//class StructData;
class DlgMerge;
class SkinClassifier;

class DlgFaceGrid : public QMainWindow
{
	Q_OBJECT

public:
	enum AutoTrackingMode {ModeBefore = 0x00000001, ModeMiddle = 0x00000002, ModeAfter = 0x00000004, ModeAll = 0x00000007};

public:
	DlgFaceGrid(QWidget *parent = 0);
	~DlgFaceGrid();

private:
	Ui::DlgFaceGridClass ui;

	std::set<int> *m_pSelectedItems;
	int *m_pLastSelected;
	StructData *m_pStructData;
	DlgMerge *m_pDlgMerge;
	//int m_popUpID;
	int m_popUpTrackID;
	int m_popUpObjectID;
	int m_popUpFrameID;

	UpdateInfo updateInfo;
	bool m_bEscape;
	//Constants::DetectType m_detectType;
	std::vector<SRegion::Type> *m_pTargetType;
	SkinClassifier *m_pSkinClassifier;
	AviManager *m_pAviManager;
	float m_scale;
	bool m_bWaitRegion;
	bool m_updateTarget;

signals:
	void sgSelectionChanged();
	void sgItemDoubleClicked(int);
	void sgItemDoubleClicked(int, int, bool);
	void sgAutoTrackingRequired(int, int);
	void sgScrollBack();
	void sgScrollForward();
	void sgManualDetect();
	void sgRkeyPressed();

protected:
	void mousePressEvent(QMouseEvent *event);
	void showEvent(QShowEvent *event);
	void keyPressEvent (QKeyEvent *event);

	void mouseDoubleClickEvent(QMouseEvent *event);

private slots:
	//void on_sliderScaleFaceGrid_valueChanged(int);
	//void on_pushButtonColorFilter_clicked();

	void on_actionSelect_Object_triggered();
	void on_actionSelect_before_cursor_triggered();
	void on_actionSelect_after_cursor_triggered();
	void on_actionAuto_Tracking_triggered();
	void on_actionMerge_triggered();
	//void on_pushButtonExport_clicked();
	//void on_pushButtonInvert_clicked();
	void on_actionExport_as_negative_samples_triggered();
	void on_actionExport_as_positive_samples_triggered();
	void on_actionJump_to_head_triggered();
	void on_actionJump_to_tail_triggered();

	//void on_checkIgnoreFrame_clicked(bool);
	void onScaleChange(float scale);
	void onWheelEvent (QWheelEvent *event);

	void on_actionMove_Up_triggered();
	void on_actionMove_Down_triggered();
	void on_actionMove_Left_triggered();
	void on_actionMove_Right_triggered();
	void on_actionEnlarge_triggered();
	void on_actionShrink_triggered();
	void on_actionDelete_Object_triggered();
	void on_actionDelete_before_this_triggered();
	void on_actionDelete_after_this_triggered();
	void on_actionIgnore_Frame_triggered(bool bIgnore);

	void on_actionExport_as_positive_samples_HS_triggered();
	void on_actionExport_as_negative_samples_HS_triggered();

public:
	void setStructData(StructData *pStructData) {pStructData->retain(); m_pStructData = pStructData; ui.faceGridView->setStructData(pStructData); m_pDlgMerge->setStructData(pStructData);}
	void setSkinClassifier(SkinClassifier *pClassifier) {m_pSkinClassifier = pClassifier;}
	void setAviManager(AviManager *pAviManager) {m_pAviManager = pAviManager; ui.faceGridView->setAviManager(pAviManager);}
	void setFrame(int f);// {ui.FaceView->setFrame(f);}
	void updateView() {ui.faceGridView->updateView();}
	//void setObjectViewSize(int sz);
	void setSelectedItems(std::set<int> *pSelectedItems);// {m_pSelectedItems = pSelectedItems;}
	void setLastSelected(int *pLastSelected);// {m_pLastSelected = pLastSelected;}
	void execMerge();
	//void setAviName(QString name) {ui.faceGridView->setAviName(name);}
	void setAviName(QString name) {ui.faceGridView->openAvi(name);}
	void eraseItems(const std::set<int> &deleteSet);
	void clearEscape() {m_bEscape = false;}
	bool isEscape() {return m_bEscape;}
	void setTargetType(std::vector<SRegion::Type> *ptargetType) {m_pTargetType = ptargetType; ui.faceGridView->setTargetType(ptargetType); m_updateTarget = true;}
	void setWaitRegion();
	void unsetWaitRegion();
	void updateTarget() {m_updateTarget = true;}

	// ÉJÉÅÉâêÿÇËë÷Ç¶
	bool changeCamera(int cameraNumber);


public slots:
	//void on_sliderScaleFace_valueChanged(int);
	//void on_sliderScaleObject_valueChanged(int);
	//void onSelectionChanged();

	void onItemClicked(int, int, int, bool, Qt::MouseButtons);
	void onItemReleased(int, int, int, bool, Qt::MouseButtons);
	//void onItemClickedFaceView(int, bool, Qt::MouseButtons);
	//void onItemClickedObjectView(int, bool, Qt::MouseButtons);
	
	//void on_actionSelect_Object_triggered();
	//void on_pushButtonMerge_clicked();
	//void on_actionAuto_Tracking_triggered();
	void onSelectionChanged();
	void onImageChanged(const std::vector<int> &changedImages, bool bSelect);

private:
	void deleteSelected();
};



#endif // DLGFACEGRID_H
