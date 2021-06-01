#include "DlgMerge.h"
#include "IplImageItem.h"
#include <QDebug>

DlgMerge::DlgMerge(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	connect(ui.FaceMergeView, SIGNAL(sgScaleChanged(float)), this, SLOT(onScaleChange(float)));
}

DlgMerge::~DlgMerge()
{

}

void DlgMerge::on_sliderScaleMerge_valueChanged(int value)
{
	float scale = float(value) / 100;
	onScaleChange(scale);
}

void DlgMerge::onScaleChange(float scale)
{
	QMatrix scaleMat(scale, 0, 0, scale, 0, 0);
	ui.FaceMergeView->setMatrix(scaleMat);
	ui.labelScaleMerge->setText(QString::number(scale * 100));
	if (ui.sliderScaleMerge->value() != scale * 100)
		ui.sliderScaleMerge->setValue(scale * 100);
}

void DlgMerge::getMapTrackToObject(std::map<int, int>& mapTrack2Object)
{
	ui.FaceMergeView->getMapTrackToObject(mapTrack2Object);
}

void DlgMerge::on_pushButtonPack_clicked()
{
	ui.FaceMergeView->pack();
	this->accept();
}