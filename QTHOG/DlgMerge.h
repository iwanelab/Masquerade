#ifndef DLGMERGE_H
#define DLGMERGE_H

#include <QDialog>
#include "ui_DlgMerge.h"

class IplImageItem;

class DlgMerge : public QDialog
{
	Q_OBJECT

public:
	DlgMerge(QWidget *parent = 0);
	~DlgMerge();

private:
	Ui::DlgMergeClass ui;

public:
	void addImageItem(IplImageItem *item) {ui.FaceMergeView->addImageItem(item);}
	void clearImageItem() {ui.FaceMergeView->clearImageItem();}
	void getMapTrackToObject(std::map<int, int>& mapTrack2Object);
	void setStructData(StructData *pStructData) {ui.FaceMergeView->setStructData(pStructData);}


private slots:
	void on_sliderScaleMerge_valueChanged(int value);
	void on_pushButtonPack_clicked();

public slots:
	void onScaleChange(float scale);
};

#endif // DLGMERGE_H
