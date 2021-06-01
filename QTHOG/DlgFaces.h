#ifndef DLGFACES_H
#define DLGFACES_H

#include <QMainWindow>
#include "ui_DlgFaces.h"
#include <QMouseEvent>
#include <set>

class StructData;
class DlgMerge;

class DlgFaces : public QMainWindow
{
	Q_OBJECT

public:
	DlgFaces(QWidget *parent = 0);
	~DlgFaces();

private:
	Ui::DlgFacesClass ui;
	std::set<int> *m_pSelectedItems;
	int *m_pLastSelected;
	StructData *m_pStructData;
	DlgMerge *m_pDlgMerge;
	int m_popUpID;

signals:
	void sgSelectionChanged();
	void sgItemDoubleClicked(int);
	void sgItemDoubleClicked(int, int, bool);
	void sgAutoTrackingRequired(int);

protected:
	void mousePressEvent(QMouseEvent *event);
	void showEvent(QShowEvent *event);
	void keyPressEvent (QKeyEvent *event);

private slots:
	void on_sliderScaleFace_valueChanged(int);
	void on_sliderScaleObject_valueChanged(int);
	void onSelectionChanged();
	//void onItemClicked(int, bool, bool, Qt::MouseButtons);
	void onItemClickedFaceView(int, bool, Qt::MouseButtons);
	void onItemClickedObjectView(int, bool, Qt::MouseButtons);
	void on_actionSelect_Object_triggered();
	void on_pushButtonMerge_clicked();
	void on_actionAuto_Tracking_triggered();

public:
	void setStructData(StructData *pStructData) {pStructData->retain(); m_pStructData = pStructData; ui.FaceView->setStructData(pStructData); ui.ObjectView->setStructData(pStructData);}
	void setAviManager(AviManager *pAviManager) {ui.FaceView->setAviManager(pAviManager); ui.ObjectView->setAviManager(pAviManager);}
	void setFrame(int f);// {ui.FaceView->setFrame(f);}
	void updateView() {ui.FaceView->update(); ui.ObjectView->updateView();}
	void setObjectViewSize(int sz);
	void setSelectedItems(std::set<int> *pSelectedItems);// {m_pSelectedItems = pSelectedItems;}
	void setLastSelected(int *pLastSelected);// {m_pLastSelected = pLastSelected;}
	void execMerge();
};

#endif // DLGFACES_H
