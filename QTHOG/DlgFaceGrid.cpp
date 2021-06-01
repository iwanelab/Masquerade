#include "DlgFaceGrid.h"
#include <QDebug>
#include "RegionData.h"
#include "IplImageItem.h"
#include <QMenu>
//#include "SkinClassifier.h"
#include <QInputDialog>
#include <QFileDialog>
#include <QScrollBar>
#include "AviManager.h"
#include "utilities.h"


DlgFaceGrid::DlgFaceGrid(QWidget *parent)
	: QMainWindow(parent),
	m_pDlgMerge(new DlgMerge(this)),
	m_bEscape(false),
	m_pStructData(0),
	//m_detectType(Constants::TypeFace),
	m_pSkinClassifier(0),
	m_pAviManager(0),
	m_scale(1.0f),
	m_bWaitRegion(false),
	m_updateTarget(true)
{
	ui.setupUi(this);
	ui.faceGridView->setParentDialog(this);

	connect(ui.faceGridView, SIGNAL(sgScaleChanged(float)), this, SLOT(onScaleChange(float)));
	connect(ui.faceGridView, SIGNAL(sgWheelEvent(QWheelEvent *)), this, SLOT(onWheelEvent(QWheelEvent *)));
	connect(ui.faceGridView, SIGNAL(sgSelectionChanged()), this, SIGNAL(sgSelectionChanged()));

	connect(ui.faceGridView, SIGNAL(sgMoveUp()), this, SLOT(on_actionMove_Up_triggered()));
	connect(ui.faceGridView, SIGNAL(sgMoveDown()), this, SLOT(on_actionMove_Down_triggered()));
	connect(ui.faceGridView, SIGNAL(sgMoveRight()), this, SLOT(on_actionMove_Right_triggered()));
	connect(ui.faceGridView, SIGNAL(sgMoveLeft()), this, SLOT(on_actionMove_Left_triggered()));
	connect(ui.faceGridView, SIGNAL(sgEnlarge()), this, SLOT(on_actionEnlarge_triggered()));
	connect(ui.faceGridView, SIGNAL(sgShrink()), this, SLOT(on_actionShrink_triggered()));
}

DlgFaceGrid::~DlgFaceGrid()
{
	if (m_pStructData)
		m_pStructData->release();
}


void DlgFaceGrid::setSelectedItems(std::set<int> *pSelectedItems)
{
	m_pSelectedItems = pSelectedItems;
	//ui.FaceView->setSelectedItems(m_pSelectedItems);
	ui.faceGridView->setSelectedItems(m_pSelectedItems);
}

void DlgFaceGrid::setLastSelected(int *pLastSelected)
{
	m_pLastSelected = pLastSelected;
	//ui.FaceView->setLastSelected(m_pLastSelected);
	ui.faceGridView->setLastSelected(m_pLastSelected);
}

void DlgFaceGrid::showEvent(QShowEvent *event)
{
	//ui.FaceGridView->alignFaces();
	ui.faceGridView->updateView();
}

void DlgFaceGrid::mousePressEvent(QMouseEvent *event)
{
	m_bEscape = true;

	m_pSelectedItems->clear();
	
	updateInfo.type = UpdateInfo::SelectionChanged;
	ui.faceGridView->updateView(false, &updateInfo);

	emit sgSelectionChanged();
}

void DlgFaceGrid::on_actionSelect_Object_triggered()
{
	m_pSelectedItems->clear();
	StructData::iterator<SRegion> itr;

	for (itr = m_pStructData->regionBegin(m_popUpObjectID/*object*/); itr != m_pStructData->regionEnd(); ++itr)
		m_pSelectedItems->insert(itr.getID());

	//ui.FaceView->alignFaces();
	updateInfo.type = UpdateInfo::SelectionChanged;
	ui.faceGridView->updateView(/*bCenterOn*/false, &updateInfo);

	emit sgSelectionChanged();
}

void DlgFaceGrid::on_actionDelete_Object_triggered()
{
	m_pSelectedItems->clear();
	StructData::iterator<SRegion> itr;

	for (itr = m_pStructData->regionBegin(m_popUpObjectID/*object*/); itr != m_pStructData->regionEnd(); ++itr)
		m_pSelectedItems->insert(itr.getID());

	deleteSelected();
}

void DlgFaceGrid::on_actionSelect_before_cursor_triggered()
{
	m_pSelectedItems->clear();
	StructData::iterator<SRegion> itr;

	for (itr = m_pStructData->regionBegin(m_popUpObjectID/*object*/); itr != m_pStructData->regionEnd(); ++itr)
	{
		if (itr->frame > m_popUpFrameID)
			break;
		m_pSelectedItems->insert(itr.getID());
	}

	//ui.FaceView->alignFaces();
	updateInfo.type = UpdateInfo::SelectionChanged;
	ui.faceGridView->updateView(/*bCenterOn*/false, &updateInfo);

	emit sgSelectionChanged();
}

void DlgFaceGrid::on_actionDelete_before_this_triggered()
{
	m_pSelectedItems->clear();
	StructData::iterator<SRegion> itr;

	for (itr = m_pStructData->regionBegin(m_popUpObjectID/*object*/); itr != m_pStructData->regionEnd(); ++itr)
	{
		if (itr->frame > m_popUpFrameID)
			break;
		m_pSelectedItems->insert(itr.getID());
	}

	deleteSelected();
}

void DlgFaceGrid::on_actionSelect_after_cursor_triggered()
{
	m_pSelectedItems->clear();
	StructData::iterator<SRegion> itr;

	for (itr = m_pStructData->regionBegin(m_popUpObjectID); itr != m_pStructData->regionEnd(); ++itr)
	{
		if (itr->frame < m_popUpFrameID)
			continue;
		m_pSelectedItems->insert(itr.getID());
	}

	//ui.FaceView->alignFaces();
	updateInfo.type = UpdateInfo::SelectionChanged;
	ui.faceGridView->updateView(/*bCenterOn*/false, &updateInfo);

	emit sgSelectionChanged();
}

void DlgFaceGrid::on_actionDelete_after_this_triggered()
{
	m_pSelectedItems->clear();
	StructData::iterator<SRegion> itr;

	for (itr = m_pStructData->regionBegin(m_popUpObjectID); itr != m_pStructData->regionEnd(); ++itr)
	{
		if (itr->frame < m_popUpFrameID)
			continue;
		m_pSelectedItems->insert(itr.getID());
	}

	deleteSelected();
}

void DlgFaceGrid::on_actionMerge_triggered()
{
	execMerge();
}

void DlgFaceGrid::execMerge()
{
	// 
	m_pDlgMerge->clearImageItem();

	for (int i = 0; i < static_cast<int>(SRegion::TypeNum); i++)
	{
		QList<QGraphicsItem*> faceItems;
		SRegion::Type targetType = static_cast<SRegion::Type>(i);
		std::map<SRegion::Type, QGraphicsItemGroup*>::iterator itTargetItemGroup = ui.faceGridView->m_pObjectItems.find(targetType);
		if (itTargetItemGroup == ui.faceGridView->m_pObjectItems.end())
			continue;
		faceItems = itTargetItemGroup->second->childItems();

		QGraphicsItem *faceItem;

		// 選択されたオブジェクトのオブジェクトID列を取得する。
		std::set<int> selectedObjects;
		foreach (faceItem, faceItems)
		{
			IplImageItem *face = qgraphicsitem_cast<IplImageItem*>(faceItem);
			//if (face->getTrackID() >= 0 && face->isSelected())
			if (face->getTrackID() >= 0 && m_pSelectedItems->find(face->getTrackID()) != m_pSelectedItems->end())
				selectedObjects.insert(face->getObjectID());
		}

		// 取得したオブジェクトID列をm_pDlgMergeへ追加する。
		foreach (faceItem, faceItems)
		{	
			IplImageItem *face = qgraphicsitem_cast<IplImageItem*>(faceItem);
			
			//if (face->getTrackID() >= 0 && m_selectedObjects.find(face->getObjectID()) != m_selectedObjects.end())
			//	qDebug() << "obj" << face->getObjectID();


			if (face->getTrackID() >= 0 /*&& face->isVisible()*/ && selectedObjects.find(face->getObjectID()) != selectedObjects.end())
				m_pDlgMerge->addImageItem(face);
		}
	}

	StructData::iterator<SRegion> itr, itNew;
	if (m_pDlgMerge->exec() == QDialog::Accepted)
	{
		std::map<int, int> mapTrack2Object;
		m_pDlgMerge->getMapTrackToObject(mapTrack2Object);

		std::set<int> deleteSet;

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
				deleteSet.insert(itr.getID());
				m_pStructData->erase(itr);
			}
		}
		
		std::vector<int> newImages;
		for (std::map<int, int>::iterator it = mapTrack2Object.begin(); it != mapTrack2Object.end(); ++it)
		{
			if (it->second >= 0)
			{
				itr = m_pStructData->regionFitAt(it->first);
				if (it->second == itr->object)
					continue;
				itNew = m_pStructData->insertRegion(itr->camera, itr->frame, it->second, itr->rect, itr->type);
				m_pStructData->setRegionFlags(itNew, itr->flags);
				newImages.push_back(itNew.getID());

				if (m_pSelectedItems->find(it->first) != m_pSelectedItems->end())
				{
					m_pSelectedItems->erase(it->first);
					m_pSelectedItems->insert(itNew.getID());

					if (*m_pLastSelected == it->first)
						*m_pLastSelected = itNew.getID();
				}
				deleteSet.insert(itr.getID());
				deleteSet.erase(itNew.getID());
				m_pStructData->erase(itr);
			}
		}

		updateInfo.type = UpdateInfo::ImageChanged;
		updateInfo.changedImages = newImages;
		updateInfo.bSelect = true;

		m_pSelectedItems->clear();

		ui.faceGridView->eraseItems(deleteSet);
		ui.faceGridView->updateView(false, &updateInfo);

		emit sgSelectionChanged();
	}
}

void DlgFaceGrid::onSelectionChanged()
{
	updateInfo.type = UpdateInfo::SelectionChanged;
	ui.faceGridView->updateView(false, &updateInfo);
}

void DlgFaceGrid::onImageChanged(const std::vector<int> &changedImages, bool bSelect)
{
	updateInfo.type = UpdateInfo::ImageChanged;
	updateInfo.changedImages = changedImages;
	updateInfo.bSelect = bSelect;

	ui.faceGridView->updateView(false, &updateInfo);
}

void DlgFaceGrid::on_actionMove_Up_triggered()
{
	if (m_pSelectedItems->empty())
		return;

	std::set<int>::iterator its;
	StructData::iterator<SRegion> itr;
	updateInfo.type = UpdateInfo::ImageChanged;
	updateInfo.changedImages.clear();

	for (its = m_pSelectedItems->begin(); its != m_pSelectedItems->end(); ++its)
	{
		if (*its < 0)
			continue;

		itr = m_pStructData->regionFitAt(*its);
		int dy = itr->rect.height / 4;
		itr->rect.y -= dy;
		if (itr->rect.y < 0)
			itr->rect.y = 0;

		updateInfo.changedImages.push_back(*its);
		//updateInfo.workFrame = itr->frame;
	}

	if (updateInfo.changedImages.empty())
		return;

	//updateInfo.bSelect = bSelect;
	ui.faceGridView->updateView(true, &updateInfo);
	emit sgSelectionChanged();
}

void DlgFaceGrid::on_actionMove_Down_triggered()
{
	if (m_pSelectedItems->empty())
		return;

	int height = m_pAviManager->getHeight();

	std::set<int>::iterator its;
	StructData::iterator<SRegion> itr;
	updateInfo.type = UpdateInfo::ImageChanged;
	updateInfo.changedImages.clear();

	for (its = m_pSelectedItems->begin(); its != m_pSelectedItems->end(); ++its)
	{
		if (*its < 0)
			continue;

		itr = m_pStructData->regionFitAt(*its);
		int dy = itr->rect.height / 4;
		itr->rect.y += dy;
		if (itr->rect.y + itr->rect.height > height)
			itr->rect.y = height - itr->rect.height;

		updateInfo.changedImages.push_back(*its);
	}
	if (updateInfo.changedImages.empty())
		return;

	//updateInfo.bSelect = bSelect;
	ui.faceGridView->updateView(true, &updateInfo);
	emit sgSelectionChanged();
}

void DlgFaceGrid::on_actionMove_Left_triggered()
{
	std::set<int>::iterator its;
	StructData::iterator<SRegion> itr;
	updateInfo.type = UpdateInfo::ImageChanged;
	updateInfo.changedImages.clear();

	for (its = m_pSelectedItems->begin(); its != m_pSelectedItems->end(); ++its)
	{
		if (*its < 0)
			continue;

		itr = m_pStructData->regionFitAt(*its);
		int dx = itr->rect.width / 4;
		itr->rect.x -= dx;
		if (itr->rect.x < 0)
			itr->rect.x = 0;

		updateInfo.changedImages.push_back(*its);
	}

	//updateInfo.bSelect = bSelect;
	ui.faceGridView->updateView(true, &updateInfo);
	emit sgSelectionChanged();
}

void DlgFaceGrid::on_actionMove_Right_triggered()
{
	int width = m_pAviManager->getWidth();

	std::set<int>::iterator its;
	StructData::iterator<SRegion> itr;
	updateInfo.type = UpdateInfo::ImageChanged;
	updateInfo.changedImages.clear();

	for (its = m_pSelectedItems->begin(); its != m_pSelectedItems->end(); ++its)
	{
		if (*its < 0)
			continue;

		itr = m_pStructData->regionFitAt(*its);
		int dx = itr->rect.width / 4;
		itr->rect.x += dx;
		if (itr->rect.x + itr->rect.width > width)
			itr->rect.x = width - itr->rect.width;

		updateInfo.changedImages.push_back(*its);
	}

	//updateInfo.bSelect = bSelect;
	ui.faceGridView->updateView(true, &updateInfo);
	emit sgSelectionChanged();
}

void DlgFaceGrid::on_actionEnlarge_triggered()
{
	int width = m_pAviManager->getWidth();
	int height = m_pAviManager->getHeight();

	std::set<int>::iterator its;
	StructData::iterator<SRegion> itr;
	updateInfo.type = UpdateInfo::ImageChanged;
	updateInfo.changedImages.clear();

	for (its = m_pSelectedItems->begin(); its != m_pSelectedItems->end(); ++its)
	{
		if (*its < 0)
			continue;

		itr = m_pStructData->regionFitAt(*its);
		CvPoint c = MathUtil::rectCenter(itr->rect);

		int newWidth = itr->rect.width * 1.5f;
		int newHeight = itr->rect.height * 1.5f;

		itr->rect.x = c.x - newWidth / 2;
		itr->rect.y = c.y - newHeight / 2;
		itr->rect.width = newWidth;
		itr->rect.height = newHeight;

		if (itr->rect.x < 0)
			itr->rect.x = 0;
		else if (itr->rect.x + itr->rect.width > width)
			itr->rect.x = width - itr->rect.width;

		if (itr->rect.y < 0)
			itr->rect.y = 0;
		else if (itr->rect.y + itr->rect.height > height)
			itr->rect.y = height - itr->rect.height;

		updateInfo.changedImages.push_back(*its);
	}

	//updateInfo.bSelect = bSelect;
	ui.faceGridView->updateView(true, &updateInfo);
	emit sgSelectionChanged();
}

void DlgFaceGrid::on_actionShrink_triggered()
{
	int width = m_pAviManager->getWidth();
	int height = m_pAviManager->getHeight();

	std::set<int>::iterator its;
	StructData::iterator<SRegion> itr;
	updateInfo.type = UpdateInfo::ImageChanged;
	updateInfo.changedImages.clear();

	for (its = m_pSelectedItems->begin(); its != m_pSelectedItems->end(); ++its)
	{
		if (*its < 0)
			continue;

		itr = m_pStructData->regionFitAt(*its);
		CvPoint c = MathUtil::rectCenter(itr->rect);

		int newWidth = itr->rect.width * 0.67f;
		int newHeight = itr->rect.height * 0.67f;

		itr->rect.x = c.x - newWidth / 2;
		itr->rect.y = c.y - newHeight / 2;
		itr->rect.width = newWidth;
		itr->rect.height = newHeight;

		if (itr->rect.x < 0)
			itr->rect.x = 0;
		else if (itr->rect.x + itr->rect.width > width)
			itr->rect.x = width - itr->rect.width;

		if (itr->rect.y < 0)
			itr->rect.y = 0;
		else if (itr->rect.y + itr->rect.height > height)
			itr->rect.y = height - itr->rect.height;

		updateInfo.changedImages.push_back(*its);
	}

	//updateInfo.bSelect = bSelect;
	ui.faceGridView->updateView(true, &updateInfo);
	emit sgSelectionChanged();
}

void DlgFaceGrid::eraseItems(const std::set<int> &deleteSet)
{
	m_pStructData->mutex.lock();
	ui.faceGridView->eraseItems(deleteSet);
	m_pStructData->mutex.unlock();

	ui.faceGridView->updateView(false);
}

void DlgFaceGrid::keyPressEvent (QKeyEvent *event)
{
	if (!event->isAutoRepeat())
		m_bEscape = true;

	if (event->key() == Qt::Key_Delete)
		deleteSelected();
	else if (event->key() == Qt::Key_Comma || event->key() == Qt::Key_A)
		emit sgScrollBack();
	else if (event->key() == Qt::Key_Period || event->key() == Qt::Key_G)
		emit sgScrollForward();
	else if (event->key() == Qt::Key_Slash || event->key() == Qt::Key_T)
		emit sgManualDetect();
	else if (event->key() == Qt::Key_R)
		emit sgRkeyPressed();
/*	
	else if (event->key() == Qt::Key_Up)// && event->modifiers() & Qt::ControlModifier)
		on_actionMove_Up_triggered();
	else if (event->key() == Qt::Key_Down && event->modifiers() & Qt::ControlModifier)
		on_actionMove_Down_triggered();
	else if (event->key() == Qt::Key_Right && event->modifiers() & Qt::ControlModifier)
		on_actionMove_Right_triggered();
	else if (event->key() == Qt::Key_Left && event->modifiers() & Qt::ControlModifier)
		on_actionMove_Left_triggered();
	else if (event->key() == Qt::Key_Up && event->modifiers() & Qt::ControlModifier && event->modifiers() & Qt::ShiftModifier)
		on_actionEnlarge_triggered();
	else if (event->key() == Qt::Key_Down && event->modifiers() & Qt::ControlModifier && event->modifiers() & Qt::ShiftModifier)
		on_actionShrink_triggered();
*/
}

void DlgFaceGrid::deleteSelected()
{
	ui.faceGridView->eraseItems(*m_pSelectedItems);

	m_pStructData->mutex.lock();

	std::set<int>::iterator its, itNext;
	for (its = m_pSelectedItems->begin(); its != m_pSelectedItems->end();)
	{
		int eraseTrack = *its;
		itNext = its;
		++itNext;

		m_pSelectedItems->erase(eraseTrack);

		if (eraseTrack < 0)
		{
			its = itNext;
			continue;
		}


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

	m_pStructData->mutex.unlock();

	emit sgSelectionChanged();

	//ui.FaceView->updateView();//alignFaces();
	ui.faceGridView->updateView();
}

void DlgFaceGrid::on_actionAuto_Tracking_triggered()
{
	//m_pSelectedItems->clear();
	StructData::iterator<SRegion> itr;

	if (m_popUpTrackID >= 0)
	{
		//itr = m_pStructData->regionFitAt(m_popUpID);
		//int object = itr->object;

		//qDebug() << object;

		m_bEscape = false;
		//ui.faceGridView->stopImageGetter();
		emit sgAutoTrackingRequired(m_popUpObjectID, /*AutoTrackingMode::*/ModeAll);
	}
	else
	{
		if (m_popUpFrameID < m_pStructData->regionBegin(m_popUpObjectID)->frame)
		{
			m_bEscape = false;
			//ui.faceGridView->stopImageGetter();
			emit sgAutoTrackingRequired(m_popUpObjectID, /*AutoTrackingMode::*/ModeBefore);
		}
		else if (m_popUpFrameID > m_pStructData->regionRBegin(m_popUpObjectID)->frame)
		{
			m_bEscape = false;
			//ui.faceGridView->stopImageGetter();
			emit sgAutoTrackingRequired(m_popUpObjectID, /*AutoTrackingMode::*/ModeAfter);
		}
		else
		{
			m_bEscape = false;
			//ui.faceGridView->stopImageGetter();
			emit sgAutoTrackingRequired(m_popUpObjectID, /*AutoTrackingMode::*/ModeMiddle);
		}
	}
}

void DlgFaceGrid::setFrame(int f)
{
	ui.faceGridView->setFrame(f, m_updateTarget);
	m_updateTarget = false;
}

void DlgFaceGrid::setWaitRegion()
{
	m_bWaitRegion = true;
	QCursor cursor;
	cursor.setShape(Qt::WhatsThisCursor);
	setCursor(cursor);
	ui.faceGridView->setWaitRegion();
}

void DlgFaceGrid::unsetWaitRegion()
{
	m_bWaitRegion = false;
	unsetCursor();
	ui.faceGridView->unsetWaitRegion();
}

bool DlgFaceGrid::changeCamera(int cameraNumber)
{
	//他に何かいる？

	return ui.faceGridView->changeCamera(cameraNumber);
}

void DlgFaceGrid::on_actionIgnore_Frame_triggered(bool bIgnore)
{
	QCursor cursor;
	cursor.setShape(Qt::WaitCursor);
	setCursor(cursor);

	ui.faceGridView->setIgnoreFrame(bIgnore);
	ui.faceGridView->updateView();

	unsetCursor();
}

void DlgFaceGrid::onScaleChange(float scale)
{
	QMatrix scaleMat(scale, 0, 0, scale, 0, 0);
	ui.faceGridView->setMatrix(scaleMat);
	/*
	ui.labelScaleFaceGrid->setText(QString::number(scale * 100));
	if (ui.sliderScaleFaceGrid->value() != scale * 100)
		ui.sliderScaleFaceGrid->setValue(scale * 100);
	*/
	m_scale = scale;
}

void DlgFaceGrid::mouseDoubleClickEvent(QMouseEvent *event)
{
	qDebug() << "double clicked";
	//m_bInDoubleClicking = true;

	//QGraphicsView::mouseDoubleClickEvent(event);
}

void DlgFaceGrid::onWheelEvent (QWheelEvent *event)
{
	 int numDegrees = event->delta() / 8;
     float numSteps = numDegrees;// / 15;

	if (event->modifiers() & Qt::ShiftModifier)
		ui.faceGridView->horizontalScrollBar()->setValue(ui.faceGridView->horizontalScrollBar()->value() + numDegrees * 8);
	else
	{
		//ui.sliderScaleFaceGrid->setValue((ui.sliderScaleFaceGrid->value() + 1) * (1 + numSteps / 100));
		onScaleChange(m_scale * (1 + numSteps / 100));
	}
}

void DlgFaceGrid::onItemClicked(int trackID, int frameID, int objectID, bool bCtrl, Qt::MouseButtons buttons)
{
	// ポップアップメニュー
	if (buttons == Qt::RightButton)
	{
		QMenu contextMenu(this);
	
		//m_popUpID = id;
		m_popUpTrackID = trackID;
		m_popUpFrameID = frameID;
		m_popUpObjectID = objectID;

		contextMenu.addAction(ui.actionSelect_Object);
		contextMenu.addAction(ui.actionAuto_Tracking);
		contextMenu.addAction(ui.actionSelect_before_cursor);
		contextMenu.addAction(ui.actionSelect_after_cursor);
		contextMenu.addAction(ui.actionJump_to_head);
		contextMenu.addAction(ui.actionJump_to_tail);
		contextMenu.addAction(ui.actionDelete_Object);
		contextMenu.addAction(ui.actionDelete_before_this);
		contextMenu.addAction(ui.actionDelete_after_this);

		if (!m_pSelectedItems->empty())
		{
			contextMenu.addAction(ui.actionMerge);

			QMenu *submenu = new QMenu("Adjust", this);
			submenu->addAction(ui.actionMove_Up);
			submenu->addAction(ui.actionMove_Down);
			submenu->addAction(ui.actionMove_Left);
			submenu->addAction(ui.actionMove_Right);
			submenu->addAction(ui.actionEnlarge);
			submenu->addAction(ui.actionShrink);
			contextMenu.addMenu(submenu);
		}
		contextMenu.exec(QCursor::pos());

		return;
	}
	else if (buttons == Qt::LeftButton)
	{
        SHORT keyState = GetKeyState(VK_SPACE);

		if (keyState & 0x8000)
		{
			//qDebug() << "space + click";
			StructData::iterator<SRegion> itr;

			if (trackID >= 0)
			{
				m_bEscape = false;
				emit sgAutoTrackingRequired(objectID, /*AutoTrackingMode::*/ModeAll);
			}
			else
			{
				if (frameID < m_pStructData->regionBegin(objectID)->frame)
				{
					m_bEscape = false;
					emit sgAutoTrackingRequired(objectID, /*AutoTrackingMode::*/ModeBefore);
				}
				else if (frameID > m_pStructData->regionRBegin(objectID)->frame)
				{
					m_bEscape = false;
					emit sgAutoTrackingRequired(objectID, /*AutoTrackingMode::*/ModeAfter);
				}
				else
				{
					m_bEscape = false;
					emit sgAutoTrackingRequired(objectID, /*AutoTrackingMode::*/ModeMiddle);
				}
			}
		}
	}
}

void DlgFaceGrid::onItemReleased(int trackID, int frameID, int objectID, bool bCtrl, Qt::MouseButtons buttons)
{
	// ポップアップメニュー
	if (buttons == Qt::RightButton)
	{
		QMenu contextMenu(this);
	
		//m_popUpID = id;
		m_popUpTrackID = trackID;
		m_popUpFrameID = frameID;
		m_popUpObjectID = objectID;

		contextMenu.addAction(ui.actionSelect_Object);
		contextMenu.addAction(ui.actionAuto_Tracking);
		contextMenu.addAction(ui.actionSelect_before_cursor);
		contextMenu.addAction(ui.actionSelect_after_cursor);
		contextMenu.addAction(ui.actionJump_to_head);
		contextMenu.addAction(ui.actionJump_to_tail);
		contextMenu.addAction(ui.actionDelete_Object);
		contextMenu.addAction(ui.actionDelete_before_this);
		contextMenu.addAction(ui.actionDelete_after_this);

		if (!m_pSelectedItems->empty())
		{
			contextMenu.addAction(ui.actionMerge);

			QMenu *submenu = new QMenu("Adjust", this);
			submenu->addAction(ui.actionMove_Up);
			submenu->addAction(ui.actionMove_Down);
			submenu->addAction(ui.actionMove_Left);
			submenu->addAction(ui.actionMove_Right);
			submenu->addAction(ui.actionEnlarge);
			submenu->addAction(ui.actionShrink);
			contextMenu.addMenu(submenu);
		}
		contextMenu.exec(QCursor::pos());

		return;
	}
	/*
	else if (buttons == Qt::LeftButton)
	{
        SHORT keyState = GetKeyState(VK_SPACE);

		if (keyState & 0x8000)
		{
			//qDebug() << "space + click";
			StructData::iterator<SRegion> itr;

			if (trackID >= 0)
			{
				m_bEscape = false;
				emit sgAutoTrackingRequired(objectID, ModeAll);
			}
			else
			{
				if (frameID < m_pStructData->regionBegin(objectID)->frame)
				{
					m_bEscape = false;
					emit sgAutoTrackingRequired(objectID, ModeBefore);
				}
				else if (frameID > m_pStructData->regionRBegin(objectID)->frame)
				{
					m_bEscape = false;
					emit sgAutoTrackingRequired(objectID,ModeAfter);
				}
				else
				{
					m_bEscape = false;
					emit sgAutoTrackingRequired(objectID, ModeMiddle);
				}
			}
		}
	}
*/
}

static float calcMdist(const PTM::TVector<2, double> &v1, const PTM::TVector<2, double> &v2,
				const PTM::TMatrixRow<2, 2, double> &m)
{
	PTM::TMatrixRow<2, 2, double> mInv = m.inv();

	PTM::TVector<2, double> dv = v2 - v1;
	PTM::TVector<2, double> mv = mInv * dv;

	double b = PTM::dot(dv, mv);

	return sqrt(b);
}



void DlgFaceGrid::on_actionExport_as_negative_samples_triggered()
{
	QStringList dirName;
    QFileDialog f;
	f.setWindowIcon(QIcon(":/Icon/AppIcon"));
    f.setFileMode(QFileDialog::DirectoryOnly);
    if (f.exec())
        dirName = f.selectedFiles();

	if (dirName.isEmpty())
		return;

	ui.faceGridView->exportSamples(dirName[0], false);
}

void DlgFaceGrid::on_actionExport_as_positive_samples_triggered()
{
	QStringList dirName;
    QFileDialog f;
	f.setWindowIcon(QIcon(":/Icon/AppIcon"));
    f.setFileMode(QFileDialog::DirectoryOnly);
    if (f.exec())
        dirName = f.selectedFiles();

	if (dirName.isEmpty())
		return;

	ui.faceGridView->exportSamples(dirName[0], true);
}

void DlgFaceGrid::on_actionExport_as_positive_samples_HS_triggered()
{
	QStringList dirName;
    QFileDialog f;
	f.setWindowIcon(QIcon(":/Icon/AppIcon"));
    f.setFileMode(QFileDialog::DirectoryOnly);
    if (f.exec())
        dirName = f.selectedFiles();

	if (dirName.isEmpty())
		return;

	ui.faceGridView->exportSamplesHeadShoulder(dirName[0], true);
}

void DlgFaceGrid::on_actionExport_as_negative_samples_HS_triggered()
{
	QStringList dirName;
    QFileDialog f;
	f.setWindowIcon(QIcon(":/Icon/AppIcon"));
    f.setFileMode(QFileDialog::DirectoryOnly);
    if (f.exec())
        dirName = f.selectedFiles();

	if (dirName.isEmpty())
		return;

	ui.faceGridView->exportSamplesHeadShoulder(dirName[0], false);
}

void DlgFaceGrid::on_actionJump_to_head_triggered()
{
	m_pSelectedItems->clear();
	StructData::iterator<SRegion> itr;

	itr = m_pStructData->regionBegin(m_popUpObjectID/*object*/);
	m_pSelectedItems->insert(itr.getID());
	*m_pLastSelected = itr.getID();

	ui.faceGridView->updateView(true);

	emit sgItemDoubleClicked(itr.getID());
}

void DlgFaceGrid::on_actionJump_to_tail_triggered()
{
	m_pSelectedItems->clear();
	StructData::reverse_iterator<SRegion> itr;

	itr = m_pStructData->regionRBegin(m_popUpObjectID/*object*/);
	m_pSelectedItems->insert(itr.getID());
	*m_pLastSelected = itr.getID();

	ui.faceGridView->updateView(true);

	emit sgItemDoubleClicked(itr.getID());
}