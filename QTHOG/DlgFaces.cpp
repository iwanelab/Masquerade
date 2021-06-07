#include "DlgFaces.h"
#include <QDebug>
#include "RegionData.h"
#include "DlgMerge.h"
#include "IplImageItem.h"

DlgFaces::DlgFaces(QWidget *parent)
	: QMainWindow(parent),
	m_pDlgMerge(new DlgMerge(this)),
	m_pStructData(0)
{
	ui.setupUi(this);

	//ui.FaceView->setSelectedItems(m_pSelectedItems);
	//ui.FaceView->setLastSelected(m_pLastSelected);
	ui.FaceView->setParentDialog(this);

	//ui.ObjectView->setSelectedItems(m_pSelectedItems);
	//ui.ObjectView->setLastSelected(m_pLastSelected);
	ui.ObjectView->setParentDialog(this);
}

void DlgFaces::setSelectedItems(std::set<int> *pSelectedItems)
{
	m_pSelectedItems = pSelectedItems;
	ui.FaceView->setSelectedItems(m_pSelectedItems);
	ui.ObjectView->setSelectedItems(m_pSelectedItems);
}

void DlgFaces::setLastSelected(int *pLastSelected)
{
	m_pLastSelected = pLastSelected;
	ui.FaceView->setLastSelected(m_pLastSelected);
	ui.ObjectView->setLastSelected(m_pLastSelected);
}

DlgFaces::~DlgFaces()
{
	if (m_pStructData)
		m_pStructData->release();
}

void DlgFaces::showEvent(QShowEvent *event)
{
	ui.FaceView->alignFaces();
	ui.ObjectView->updateView();
}

void DlgFaces::setObjectViewSize(int sz)
{
	QList<int> sizes;
	int frameHeight = ui.FaceObjectSplitter->frameRect().height();

	if (sz > frameHeight / 2)
		sz = frameHeight / 2;

	sizes.push_back(frameHeight - sz);
	sizes.push_back(sz);

	//qDebug() << "setSizes" << frameHeight - sz << sz;

	ui.FaceObjectSplitter->setSizes(sizes);
}

void DlgFaces::on_sliderScaleFace_valueChanged(int scale)
{
	ui.labelScaleFace->setText(QString::number(scale));
	ui.FaceView->setScale((float)scale / 100.0f);
}

void DlgFaces::on_sliderScaleObject_valueChanged(int scale)
{
	ui.labelScaleObject->setText(QString::number(scale));
	ui.ObjectView->setScale((float)scale / 100.0f);
	//ui.FaceView->setScale((float)scale / 100.0f);
}

void DlgFaces::mousePressEvent(QMouseEvent *event)
{
	m_pSelectedItems->clear();
	ui.FaceView->alignFaces();
	ui.ObjectView->updateView();

	emit sgSelectionChanged();
}

void DlgFaces::onItemClickedFaceView(int id, bool bCtrl, Qt::MouseButtons buttons)
{
	if (buttons != Qt::LeftButton)
		return;

	if (!bCtrl)
		m_pSelectedItems->clear();

	if (m_pSelectedItems->find(id) == m_pSelectedItems->end())
	{
		m_pSelectedItems->insert(id);
		*m_pLastSelected = id;
	}
	else
	{
		m_pSelectedItems->erase(id);
		if (m_pSelectedItems->empty())
			*m_pLastSelected = -1;
		else
			*m_pLastSelected = *(m_pSelectedItems->begin());
	}

	ui.FaceView->alignFaces();
	ui.ObjectView->updateView(/*bCenterOn*/true);

	emit sgSelectionChanged();
}

void DlgFaces::onItemClickedObjectView(int id, bool bCtrl, Qt::MouseButtons buttons)
{
	// ポップアップメニュー
	if (buttons == Qt::RightButton)
	{
		QMenu contextMenu(this);
	
		m_popUpID = id;

		contextMenu.addAction(ui.actionSelect_Object);
		contextMenu.addAction(ui.actionAuto_Tracking);
		contextMenu.exec(QCursor::pos());
/*
		if (m_pSelectedItems->empty())
		{
			m_pSelectedItems->insert(id);
			*m_pLastSelected = id;
		}
*/
		return;
	}
	else if (buttons == Qt::LeftButton)
	{
		if (!bCtrl)
			m_pSelectedItems->clear();

		if (m_pSelectedItems->find(id) == m_pSelectedItems->end())
		{
			m_pSelectedItems->insert(id);
			*m_pLastSelected = id;
		}
		else
		{
			m_pSelectedItems->erase(id);
			if (m_pSelectedItems->empty())
				*m_pLastSelected = -1;
			else
				*m_pLastSelected = *(m_pSelectedItems->begin());
		}

		ui.FaceView->alignFaces();
		ui.ObjectView->updateView(/*bCenterOn*/false);

		emit sgSelectionChanged();
	}
}

void DlgFaces::on_actionSelect_Object_triggered()
{
	m_pSelectedItems->clear();
	StructData::iterator<SRegion> itr;

	itr = m_pStructData->regionFitAt(m_popUpID);
	int object = itr->object;

	for (itr = m_pStructData->regionBegin(object); itr != m_pStructData->regionEnd(); ++itr)
		m_pSelectedItems->insert(itr.getID());

	ui.FaceView->alignFaces();
	ui.ObjectView->updateView(/*bCenterOn*/false);

	emit sgSelectionChanged();
}

void DlgFaces::onSelectionChanged()
{
	ui.FaceView->updateView();//alignFaces();
	ui.ObjectView->updateView();
}

void DlgFaces::keyPressEvent (QKeyEvent *event)
{
	if (event->key() == Qt::Key_Delete)
	{
		std::set<int>::iterator its, itNext;
		for (its = m_pSelectedItems->begin(); its != m_pSelectedItems->end();)
		{
			int eraseTrack = *its;
			itNext = its;
			++itNext;
			m_pSelectedItems->erase(eraseTrack);
			if (*m_pLastSelected == eraseTrack)
			{
				if (m_pSelectedItems->empty())
					*m_pLastSelected = -1;
				else
					*m_pLastSelected = *(m_pSelectedItems->begin());
			}
			m_pStructData->erase(m_pStructData->regionFitAt(eraseTrack));
			its = itNext;
		}
		emit sgSelectionChanged();

		ui.FaceView->updateView();//alignFaces();
		ui.ObjectView->updateView();
	}
}

void DlgFaces::on_pushButtonMerge_clicked()
{
	execMerge();
}

void DlgFaces::execMerge()
{
	m_pDlgMerge->clearImageItem();

	QList<QGraphicsItem *> faceItems = ui.ObjectView->scene()->items();
	QGraphicsItem *faceItem;

	std::set<int> m_selectedObjects;
	foreach (faceItem, faceItems)
	{
		IplImageItem *face = (IplImageItem*)faceItem;
		if (face->getTrackID() >= 0 && face->isVisible())
			m_selectedObjects.insert(face->getObjectID());
	}

	foreach (faceItem, faceItems)
	{
		IplImageItem *face = (IplImageItem*)faceItem;
		if (face->getTrackID() >= 0 && face->isVisible() && m_selectedObjects.find(face->getObjectID()) != m_selectedObjects.end())
			m_pDlgMerge->addImageItem(face);
	}

	StructData::iterator<SRegion> itr, itNew;
	if (m_pDlgMerge->exec() == QDialog::Accepted)
	{
		std::map<int, int> mapTrack2Object;
		m_pDlgMerge->getMapTrackToObject(mapTrack2Object);

		for (std::map<int, int>::iterator it = mapTrack2Object.begin(); it != mapTrack2Object.end(); ++it)
		{
			if (it->second < 0)
			{
				m_pSelectedItems->erase(it->first);
				if (*m_pLastSelected == it->first)
				{
					if (m_pSelectedItems->empty())
						*m_pLastSelected = -1;
					else
						*m_pLastSelected = *(m_pSelectedItems->begin());
				}

				itr = m_pStructData->regionFitAt(it->first);
				m_pStructData->erase(itr);
			}
		}
		
		for (std::map<int, int>::iterator it = mapTrack2Object.begin(); it != mapTrack2Object.end(); ++it)
		{
			if (it->second >= 0)
			{
				itr = m_pStructData->regionFitAt(it->first);
				if (it->second == itr->object)
					continue;

				itNew = m_pStructData->insertRegion(itr->camera, itr->frame, it->second, itr->rect, itr->type);
				m_pStructData->setRegionFlags(itNew, itr->flags);

				if (m_pSelectedItems->find(it->first) != m_pSelectedItems->end())
				{
					m_pSelectedItems->erase(it->first);
					m_pSelectedItems->insert(itNew.getID());

					if (*m_pLastSelected == it->first)
						*m_pLastSelected = itNew.getID();
				}
				m_pStructData->erase(itr);
			}
		}

		ui.FaceView->alignFaces();
		ui.ObjectView->updateView();

		emit sgSelectionChanged();
	}
}

void DlgFaces::on_actionAuto_Tracking_triggered()
{
	//m_pSelectedItems->clear();
	StructData::iterator<SRegion> itr;

	itr = m_pStructData->regionFitAt(m_popUpID);
	int object = itr->object;

	qDebug() << object;

	emit sgAutoTrackingRequired(object);
}

void DlgFaces::setFrame(int f)
{
	/*
	m_pSelectedItems->clear();

	StructData::iterator<SRegion> itr;

	for (itr = m_pStructData->regionBegin(0, f); itr != m_pStructData->regionEnd(); ++itr)
	{
		//int object = itr->object;
		if (itr->type != SRegion::FACE)
			continue;
		m_pSelectedItems->insert(itr.getID());
	}
	*/
	ui.FaceView->setFrame(f);
	//ui.ObjectView->updateView();
}