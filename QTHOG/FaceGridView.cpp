#include "FaceGridView.h"
#include "FrameFaceScene.h"
#include "RegionData.h"
#include "AviManager.h"
#include "DlgFaceGrid.h"
#include "QFaceImageGetter.h"
#include <QGraphicsItem>
#include <QDebug>
#include "AviManager.h"
#include <QFileInfo>
#include <QDir>
//#include <QtGui>
/*
static std::string UnicodeToMultiByte(const std::wstring& Source, UINT CodePage = CP_ACP, DWORD Flags = 0)
{
  std::vector<char> Dest(::WideCharToMultiByte(CodePage, Flags, Source.c_str(), static_cast<int>(Source.size()), NULL, 0, NULL, NULL));
  return std::string(Dest.begin(),
    Dest.begin() + ::WideCharToMultiByte(CodePage, Flags, Source.c_str(), static_cast<int>(Source.size()), &Dest[0], static_cast<int>(Dest.size()), NULL, NULL));
}
*/

FaceGridView::FaceGridView(QWidget *parent)
	: FaceView(parent),
	m_scale(1),
	m_curFrame(-1),
	//m_curCamera(0),
	m_bIgnoreFrame(false),
	m_bMidDrag(false),
	m_bRightDrag(false),
	m_dragMode(NONE),
	//m_pFaceImageGetter(0),
	m_bInDoubleClicking(false),
	//m_detectType(Constants::TypeFace),
	//m_pAviManager(new AviManager),
	m_bWaitRegion(false)
{
	for (int i = 0; i < static_cast<int>(SRegion::TypeNum); i++)
	{
		SRegion::Type targetType = static_cast<SRegion::Type>(i);
		m_pObjectItems[targetType] = new QGraphicsItemGroup();
		m_pObjectItems[targetType]->setHandlesChildEvents(false);
		scene()->addItem(m_pObjectItems[targetType]);
		m_pObjectItems[targetType]->show();
	}

	m_pArrowItems = new QGraphicsItemGroup();
	m_pArrowItems->setHandlesChildEvents(false);
	scene()->addItem(m_pArrowItems);
	m_pArrowItems->show();

	m_pFaceImageGetter = new QFaceImageGetter(this, &m_pObjectItems, m_pStructData);
	connect(m_pFaceImageGetter, SIGNAL(sgUpdateImage()), this, SLOT(onUpdateImage()), Qt::QueuedConnection);
}

FaceGridView::~FaceGridView()
{
	//while (m_pFaceImageGetter)
	while (m_pFaceImageGetter->isRunning())
	{
		m_pFaceImageGetter->stop();

		if (m_pFaceImageGetter->wait(50))// || !m_pFaceImageGetter->isRun())
		{
			delete m_pFaceImageGetter;
			m_pFaceImageGetter = 0;
		}
	}

	//if (!m_pCapture)
	//	cvReleaseCapture(&m_pCapture);
	//delete m_pAviManager;
}

void FaceGridView::keyPressEvent (QKeyEvent *event)
{
	if (event->key() == Qt::Key_Up && event->modifiers() & Qt::ControlModifier && event->modifiers() & Qt::ShiftModifier)
		emit sgEnlarge();
	else if (event->key() == Qt::Key_Down && event->modifiers() & Qt::ControlModifier && event->modifiers() & Qt::ShiftModifier)
		emit sgShrink();
	else if (event->key() == Qt::Key_Up && event->modifiers() & Qt::ControlModifier)
		emit sgMoveUp();
	else if (event->key() == Qt::Key_Down && event->modifiers() & Qt::ControlModifier)
		emit sgMoveDown();
	else if (event->key() == Qt::Key_Right && event->modifiers() & Qt::ControlModifier)
		emit sgMoveRight();
	else if (event->key() == Qt::Key_Left && event->modifiers() & Qt::ControlModifier)
		emit sgMoveLeft();
	else
		QGraphicsView::keyPressEvent(event);
}

void FaceGridView::openAvi(QString filename)
{
	//if (m_pCapture)
	//	cvReleaseCapture(&m_pCapture);

	//m_pCapture = cvCaptureFromFile(filename.toLocal8Bit());
	m_pAviManager->openAvi(filename);
}

void FaceGridView::calcGrid(int frame)
{
	m_firstFrame = m_lastFrame = -1;
	std::set<int>::iterator its;
	StructData::iterator<SRegion> itr;

	m_curObjects.clear();

	if (m_bIgnoreFrame)
	{
		for (int frame = 0; frame < m_pAviManager->getFrameCount(); ++frame)
		{
			for (itr = m_pStructData->regionBegin(m_camera, frame); itr != m_pStructData->regionEnd(); ++itr)
			{
				if (std::find(m_pTargetType->begin(), m_pTargetType->end(), itr->type) == m_pTargetType->end())
					continue;
				m_curObjects.insert(itr->object);
			}
		}
	}
	else
	{
		for (itr = m_pStructData->regionBegin(m_camera, frame); itr != m_pStructData->regionEnd(); ++itr)
		{
			if (std::find(m_pTargetType->begin(), m_pTargetType->end(), itr->type) == m_pTargetType->end())
				continue;
			m_curObjects.insert(itr->object);
		}
	}

	m_gridX.clear();
	m_gridY.clear();
	m_gridWidth.clear();
	m_gridHeight.clear();

	int frameCount = m_pAviManager->getFrameCount();
/*
	if (frameCount == 0)
		m_gridWidth.resize(1, FaceMargin);
	else
		m_gridWidth.resize(frameCount, FaceMargin);

	//for (its = m_selectedObjects.begin(); its != m_selectedObjects.end(); ++its)
	int count = 0;
	for (its = m_curObjects.begin(); its != m_curObjects.end(); ++its)
	{
		if (m_gridY.empty())
			m_gridY.push_back(0);
		else
			m_gridY.push_back(m_gridY[m_gridY.size() - 1] + m_gridHeight.back());

		m_gridHeight.push_back(0);
		int &maxHeight = m_gridHeight.back();

		for (itr = m_pStructData->regionBegin(*its); itr != m_pStructData->regionEnd(); ++itr)
		{
			if (itr->type != SRegion::FACE)
				continue;

			if (itr->rect.height + FaceMargin > maxHeight)
				maxHeight = itr->rect.height + FaceMargin;

			if (itr->rect.width + FaceMargin > m_gridWidth[itr->frame])
				m_gridWidth[itr->frame] = itr->rect.width + FaceMargin;

			if (m_firstFrame < 0 || itr->frame < m_firstFrame)
				m_firstFrame = itr->frame;
			if (m_lastFrame < 0 || itr->frame > m_lastFrame)
				m_lastFrame = itr->frame;
		}
	}

	m_gridX.push_back(0);
	for (int i = 1; i < m_gridWidth.size(); ++i)
		m_gridX.push_back(m_gridX.back() + m_gridWidth[i - 1]);
*/

	m_gridY.resize(m_curObjects.size(), 0);
	m_gridHeight.resize(m_curObjects.size(), FaceDispSize + 2 * FaceMargin);
	int count = 0;
	for (its = m_curObjects.begin(); its != m_curObjects.end(); ++its)
	{
		if (m_firstFrame < 0 || m_pStructData->regionBegin(*its)->frame < m_firstFrame)
			m_firstFrame = m_pStructData->regionBegin(*its)->frame;

		if (m_lastFrame < 0 || m_pStructData->regionRBegin(*its)->frame > m_lastFrame)
			m_lastFrame = m_pStructData->regionRBegin(*its)->frame;

		if (count > 0)
			m_gridY[count] = m_gridY[count - 1] + FaceDispSize + 2 * FaceMargin;

		++count;
	}

	if (m_curFrame > m_lastFrame)
		m_lastFrame = m_curFrame;

	if (m_curFrame < m_firstFrame)
		m_firstFrame = m_curFrame;

	if (m_firstFrame > 0)
		m_firstFrame--;
	if (m_lastFrame < m_pAviManager->getFrameCount() - 1)
		m_lastFrame++;

	m_gridX.resize(m_lastFrame - m_firstFrame + 1, 0);
	m_gridWidth.resize(m_lastFrame - m_firstFrame + 1, FaceDispSize + 2 * FaceMargin);

	for (int i = 1; i < m_gridX.size(); ++i)
		m_gridX[i] = m_gridX[i - 1] + FaceDispSize + 2 * FaceMargin;
}

void FaceGridView::drawBackground(QPainter *painter, const QRectF &rect)
{
	QPointF tl = mapToScene(0, 0);
	QPointF br = mapToScene(this->rect().width(), this->rect().height());

	painter->fillRect(tl.x(), tl.y(), br.x() - tl.x(), br.y() - tl.y(), QColor(70, 70, 70));

	if (m_gridHeight.empty())
		return;

	int totalWidth = m_gridX.back() + m_gridWidth.back();
	int totalHeight = m_gridY.back() + m_gridHeight.back();

	int prevBottom = 0;
	int gray = 75;
	for (int i = 0; i < m_gridHeight.size(); ++i)
	{
		painter->fillRect(0, prevBottom, totalWidth, m_gridHeight[i], QColor(gray, gray, gray));
		prevBottom += m_gridHeight[i];
		gray = (gray == 75) ? 80 : 75;
	}

	if (m_curFrame >= 0)
		painter->fillRect(m_gridX[m_curFrame - m_firstFrame], 0, m_gridWidth[m_curFrame - m_firstFrame], totalHeight, QColor(160, 160, 160));

	QPen pen;
	pen.setColor(QColor(70, 70, 70));
	painter->setPen(pen);
	
	for (int i = 1; i < m_gridX.size(); ++i)
		painter->drawLine(m_gridX[i], 0, m_gridX[i], totalHeight);
}

void FaceGridView::setScale(float scale)
{
	QMatrix scaleMat(scale, 0, 0, scale, 0, 0);
	setMatrix(scaleMat);
	m_scale = scale;
}

void FaceGridView::stopImageGetter()
{
	// スレッドを停止する
	while (m_pFaceImageGetter->isRunning())
		m_pFaceImageGetter->stop();
}

void FaceGridView::eraseItems(const std::set<int> &eraseList)
{
	// スレッドを停止する
	while (m_pFaceImageGetter->isRunning())
	{
		m_pFaceImageGetter->stop();
	}

	//QGraphicsItem *item;
	//IplImageItem *face;

	std::vector<QGraphicsItem*> deleteItem;
	 
	for (int i = 0; i < static_cast<int>(SRegion::TypeNum); i++)
	{
		SRegion::Type targetType = static_cast<SRegion::Type>(i);
		std::map<SRegion::Type, QGraphicsItemGroup*>::iterator itTargetItemGroup = m_pObjectItems.find(targetType);
		if (itTargetItemGroup == m_pObjectItems.end())
			continue;
		foreach (QGraphicsItem *item, itTargetItemGroup->second->childItems())
		{
			IplImageItem *object = qgraphicsitem_cast<IplImageItem*>(item);
			if (eraseList.find(object->getTrackID()) != eraseList.end())
				deleteItem.push_back(item);
		}
	}

/*
	std::set<int>::const_iterator its;
	for (its = eraseList.begin(); its != eraseList.end(); ++its)
	{
		m_mapFaceItems.erase(*its);
		m_mapPlateItems.erase(*its);
	}
*/
	for (int i = 0; i < deleteItem.size(); ++i)
		delete deleteItem[i];

	for (int i = 0; i < static_cast<int>(SRegion::TypeNum); i++)
	{
		SRegion::Type targetType = static_cast<SRegion::Type>(i);
		std::map<SRegion::Type, std::map<int, int>>::iterator itMapObjectItem = m_mapObjectItems.find(targetType);
		if (itMapObjectItem == m_mapObjectItems.end())
			continue;
		itMapObjectItem->second.clear();
		int count = 0;
		std::map<SRegion::Type, QGraphicsItemGroup*>::iterator itTargetItemGroup = m_pObjectItems.find(targetType);
		if (itTargetItemGroup == m_pObjectItems.end())
			continue;
		foreach (QGraphicsItem *item, itTargetItemGroup->second->childItems())
		{
			IplImageItem *object = qgraphicsitem_cast<IplImageItem*>(item);
			itMapObjectItem->second[object->getTrackID()] = count;
			count++;
		}
	}
}

IplImageItem* FaceGridView::getViewObject(QGraphicsItemGroup *pGraphicsItems, StructData::iterator<SRegion>& region)
{
	if (m_mapObjectItems[region->type].find(region.getID()) != m_mapObjectItems[region->type].end())
		return qgraphicsitem_cast<IplImageItem*>(pGraphicsItems->childItems()[m_mapObjectItems[region->type][region.getID()]]);
	// 描画一覧に存在しないオブジェクト番号なら描画アイテムを作成して描画一覧に追加する。
	//描画一覧に追加
	m_mapObjectItems[region->type][region.getID()] = pGraphicsItems->childItems().size();

	IplImageItem* object = new IplImageItem(pGraphicsItems);

	object->setTrackIndices(region.getID(), region->frame, region->object);
	object->setType(IplImageItem::IMAGE);

	object->setCenterOnClicked(false);
	// ダブルクリックイベントを登録
	connect(object, SIGNAL(sgItemDoubleClicked(int)), m_pParentDialog, SIGNAL(sgItemDoubleClicked(int)));
	connect(object, SIGNAL(sgItemDoubleClicked(int, int, bool)), m_pParentDialog, SIGNAL(sgItemDoubleClicked(int, int, bool)));
	return object;
}

void FaceGridView::addImageAcquisitionList(std::map<int, std::vector<std::pair<SRegion::Type, int>>> &forwardItemList, std::map<int, std::vector<std::pair<SRegion::Type, int>>> &backwardItemList, int objectId, IplImageItem* object, StructData::iterator<SRegion>& region)
{
	if (!object->checkImageAvailable())
	{	// 描画状態が無効なら
		// 画像をクリアし、リストに追加する。
		object->clearImage();
		if (region->frame >= m_curFrame)
			forwardItemList[region->frame - m_curFrame].push_back(std::make_pair(region->type, objectId));
		else
			backwardItemList[m_curFrame - region->frame].push_back(std::make_pair(region->type, objectId));

		// スケールを調整
		float scale;
		if (region->rect.height > region->rect.width)
			scale = (float)FaceDispSize / (float)region->rect.height;
		else
			scale = (float)FaceDispSize / (float)region->rect.width;

		object->setScale(scale);

		if (m_pStructData->checkRegionFlags(region, SRegion::FlagTracked))
			object->setFrameColor(QColor(255, 0, 255));
		else
			object->setFrameColor(QColor(0, 255, 255));
	}
}

void FaceGridView::updateView(bool bCenterOn, UpdateInfo *info)
{
	static bool busy = false;
	if (busy) return;
	busy = true;
	int frameCount = m_pAviManager->getFrameCount();

	//QGraphicsItem *item;
	//IplImageItem *face, *plate;

	// 描画用画像テーブルに選択／非選択の状態を設定する。
	for (int i = 0; i < static_cast<int>(SRegion::TypeNum); i++)
	{
		SRegion::Type targetType = static_cast<SRegion::Type>(i);
		std::map<SRegion::Type, QGraphicsItemGroup*>::iterator itTargetItemGroup = m_pObjectItems.find(targetType);
		if (itTargetItemGroup == m_pObjectItems.end())
			continue;
		foreach (QGraphicsItem *item, itTargetItemGroup->second->childItems())
		{
			IplImageItem *object = qgraphicsitem_cast<IplImageItem*>(item);
			object->setSelected(m_pSelectedItems->find(object->getTrackID()) != m_pSelectedItems->end());
		}
	}

	// オブジェクト選択の変更のみではない場合、指定されたオブジェクトの再描画を行う。
	if (!info || info->type != UpdateInfo::SelectionChanged)
	{
		// 全てのオブジェクトを非表示にしている？
		for (int i = 0; i < static_cast<int>(SRegion::TypeNum); i++)
		{
			SRegion::Type targetType = static_cast<SRegion::Type>(i);
			std::map<SRegion::Type, QGraphicsItemGroup*>::iterator itTargetItemGroup = m_pObjectItems.find(targetType);
			if (itTargetItemGroup == m_pObjectItems.end())
				continue;
			foreach (QGraphicsItem *item, itTargetItemGroup->second->childItems())
				item->hide();
		}
		foreach (QGraphicsItem *item, m_pArrowItems->childItems())
			item->hide();

		// 要内容確認
		calcGrid(m_curFrame);

		// 描画範囲を指定
		QRectF visibleArea = mapToScene(viewport()->rect()).boundingRect();;
		QList<QRectF> rects;
		rects.append(visibleArea);

		m_pStructData->mutex.lock();
		QGraphicsView::updateScene(rects);
		m_pStructData->mutex.unlock();

		if (m_curObjects.empty())
		{
			busy = false;
			return;
		}

		int totalWidth = m_gridX.back() + m_gridWidth.back();
		int totalHeight = m_gridY.back() + m_gridHeight.back();

		scene()->setSceneRect(0, 0, totalWidth, totalHeight);
	}

	// 描画対象オブジェクトが無ければ終了する。
	if (m_curObjects.empty())
	{
		busy = false;
		return;
	}

	// 描画イメージの変更（サイズ、位置）のみの場合
	if (info && info->type == UpdateInfo::ImageChanged)
	{
		// イメージ変更したデータのループ
		for (int i = 0; i < info->changedImages.size(); ++i)
		{
			for (int typeNo = 0; typeNo < static_cast<int>(SRegion::TypeNum); typeNo++)
			{
				SRegion::Type targetType = static_cast<SRegion::Type>(typeNo);
				std::map<SRegion::Type, QGraphicsItemGroup*>::iterator itTargetItemGroup = m_pObjectItems.find(targetType);
				if (itTargetItemGroup == m_pObjectItems.end())
					continue;
				std::map<SRegion::Type, std::map<int, int>>::iterator itTargetMapItem = m_mapObjectItems.find(targetType);
				if (itTargetMapItem == m_mapObjectItems.end())
					continue;
				if (itTargetMapItem->second.find(info->changedImages[i]) != itTargetMapItem->second.end())
				{	// 描画一覧に存在しない。
					// 描画状態は無効
					IplImageItem *object = qgraphicsitem_cast<IplImageItem*>(itTargetItemGroup->second->childItems()[itTargetMapItem->second[info->changedImages[i]]]);
					object->setImageAvailable(false);
				}
			}
		}
	}

	std::set<int>::iterator its;
	StructData::iterator<SRegion> itr;

	int itemCount = 0;
	int objIndex = 0;

	std::map<int, std::vector<std::pair<SRegion::Type, int>>> forwardItemList;
	std::map<int, std::vector<std::pair<SRegion::Type, int>>> backwardItemList;

	// オブジェクト選択の変更のみではない場合、
	if (!info || info->type != UpdateInfo::SelectionChanged)
	{
		int arrowItemCount = 0;

		// 描画対象全オブジェクトのループ
		for (its = m_curObjects.begin(); its != m_curObjects.end(); ++its)
		{
			int obj = *its;

			int tmpFirstFrame = -1, tmpLastFrame = -1;	// オブジェクトで使用しているフレーム範囲
			std::vector<int> lostFrames;				// フレーム範囲内でオブジェクトが使用していないフレーム列

			// 対象オブジェクト内の全リージョン（フレーム）のループ
			for (itr = m_pStructData->regionBegin(*its); itr != m_pStructData->regionEnd(); ++itr)
			{
				if (std::find(m_pTargetType->begin(), m_pTargetType->end(), itr->type) == m_pTargetType->end())
					continue;
				// フレーム範囲を算出
				int frame = itr->frame;
				//qDebug() << "obj" << obj << frame;

				if (tmpFirstFrame < 0)
					tmpFirstFrame = frame;
				else
				{
					for (int lostFrame = tmpLastFrame + 1; lostFrame < frame; ++lostFrame)
						lostFrames.push_back(lostFrame);
				}

				tmpLastFrame = frame;

				std::map<SRegion::Type, QGraphicsItemGroup*>::iterator itTargetItemGroup = m_pObjectItems.find(itr->type);
				if (itTargetItemGroup == m_pObjectItems.end())
					continue;
				//std::map<SRegion::Type, std::map<int, int>>::iterator itTargetMapItem = m_mapObjectItems.find(itr->type);
				//if (itTargetMapItem == m_mapObjectItems.end())
				//	continue;
				IplImageItem* object = getViewObject(itTargetItemGroup->second, itr);
				addImageAcquisitionList(forwardItemList, backwardItemList, m_mapObjectItems[itr->type][itr.getID()], object, itr);

				// 描画する位置を計算
				float centerX = m_gridX[frame - m_firstFrame] + (float)m_gridWidth[frame - m_firstFrame] / 2;
				float centerY = m_gridY[objIndex] + (float)m_gridHeight[objIndex] / 2;

				// 描画
				object->setPos(centerX, centerY);
				object->setTrackIndices(itr.getID(), itr->frame, itr->object);
				object->setType(IplImageItem::IMAGE);
				object->show();

				if (bCenterOn && *m_pLastSelected >= 0 && *m_pLastSelected == itr.getID())
				{
					m_pStructData->mutex.lock();
					centerOn(centerX, centerY);
					m_pStructData->mutex.unlock();
				}
			}	// for (itr = m_pStructData->regionBegin(*its); itr != m_pStructData->regionEnd(); ++itr)

			if (tmpFirstFrame < 0)
				continue;
			// オブジェクトリストの左側に前方トラッキング用アイテムを追加する。
			if (tmpFirstFrame > 0)
			{
				++arrowItemCount;
				IplImageItem *arrow;
				if (m_pArrowItems->childItems().size() < arrowItemCount)
				{
					arrow = new IplImageItem(m_pArrowItems);
					arrow->setCenterOnClicked(false);
					//connect(arrow, SIGNAL(sgItemClicked(int, int, int, bool, Qt::MouseButtons)), m_pParentDialog, SLOT(onItemClicked(int, int, int, bool, Qt::MouseButtons)));
					//connect(arrow, SIGNAL(sgItemReleased(int, int, int, bool, Qt::MouseButtons)), m_pParentDialog, SLOT(onItemReleased(int, int, int, bool, Qt::MouseButtons)));
					connect(arrow, SIGNAL(sgItemDoubleClicked(int)), m_pParentDialog, SIGNAL(sgItemDoubleClicked(int)));
					connect(arrow, SIGNAL(sgItemDoubleClicked(int, int, bool)), m_pParentDialog, SIGNAL(sgItemDoubleClicked(int, int, bool)));
					//scene()->addItem(arrow);
				}
				else
					arrow = qgraphicsitem_cast<IplImageItem*>(m_pArrowItems->childItems()[arrowItemCount - 1]);
					//arrow = (IplImageItem*)(scene()->items()[itemCount - 1]);

				arrow->setTrackIndices(-1, tmpFirstFrame - 1, *its);
				arrow->setType(IplImageItem::LARROW);
				arrow->setFrameColor(QColor(0, 255, 0));
				float centerX = m_gridX[tmpFirstFrame - 1 - m_firstFrame] + (float)m_gridWidth[tmpFirstFrame - 1 - m_firstFrame] / 2;
				float centerY = m_gridY[objIndex] + (float)m_gridHeight[objIndex] / 2;
				arrow->setPos(centerX, centerY);
				arrow->show();
			}

			// オブジェクトリストの右側に後方トラッキング用アイテムを追加する。
			if (tmpLastFrame < frameCount - 1)
			{
				++arrowItemCount;
				IplImageItem *arrow;
				//if (scene()->items().size() < arrowItemCount)
				if (m_pArrowItems->childItems().size() < arrowItemCount)
				{
					arrow = new IplImageItem(m_pArrowItems);
					arrow->setCenterOnClicked(false);
					//connect(arrow, SIGNAL(sgItemClicked(int, int, int, bool, Qt::MouseButtons)), m_pParentDialog, SLOT(onItemClicked(int, int, int, bool, Qt::MouseButtons)));
					//connect(arrow, SIGNAL(sgItemReleased(int, int, int, bool, Qt::MouseButtons)), m_pParentDialog, SLOT(onItemReleased(int, int, int, bool, Qt::MouseButtons)));
					connect(arrow, SIGNAL(sgItemDoubleClicked(int)), m_pParentDialog, SIGNAL(sgItemDoubleClicked(int)));
					connect(arrow, SIGNAL(sgItemDoubleClicked(int, int, bool)), m_pParentDialog, SIGNAL(sgItemDoubleClicked(int, int, bool)));
					//scene()->addItem(arrow);
				}
				else
					arrow = qgraphicsitem_cast<IplImageItem*>(m_pArrowItems->childItems()[arrowItemCount - 1]);
					//arrow = (IplImageItem*)(scene()->items()[itemCount - 1]);

				arrow->setTrackIndices(-1, tmpLastFrame + 1, *its);
				arrow->setType(IplImageItem::RARROW);
				arrow->setFrameColor(QColor(0, 255, 0));
				float centerX = m_gridX[tmpLastFrame + 1 - m_firstFrame] + (float)m_gridWidth[tmpLastFrame + 1 - m_firstFrame] / 2;
				float centerY = m_gridY[objIndex] + (float)m_gridHeight[objIndex] / 2;
				arrow->setPos(centerX, centerY);
				arrow->show();
			}

			// 間が抜けているフレームにトラッキング用アイテムを追加する。
			for (int i = 0; i < lostFrames.size(); ++i)
			{
				++arrowItemCount;
				IplImageItem *arrow;
				//if (scene()->items().size() < arrowItemCount)
				if (m_pArrowItems->childItems().size() < arrowItemCount)
				{
					arrow = new IplImageItem(m_pArrowItems);
					arrow->setCenterOnClicked(false);
					//connect(arrow, SIGNAL(sgItemClicked(int, int, int, bool, Qt::MouseButtons)), m_pParentDialog, SLOT(onItemClicked(int, int, int, bool, Qt::MouseButtons)));
					//connect(arrow, SIGNAL(sgItemReleased(int, int, int, bool, Qt::MouseButtons)), m_pParentDialog, SLOT(onItemReleased(int, int, int, bool, Qt::MouseButtons)));
					connect(arrow, SIGNAL(sgItemDoubleClicked(int)), m_pParentDialog, SIGNAL(sgItemDoubleClicked(int)));
					connect(arrow, SIGNAL(sgItemDoubleClicked(int, int, bool)), m_pParentDialog, SIGNAL(sgItemDoubleClicked(int, int, bool)));
					//scene()->addItem(newItem);
				}
				else
					arrow = qgraphicsitem_cast<IplImageItem*>(m_pArrowItems->childItems()[arrowItemCount - 1]);

				arrow->setTrackIndices(-1, lostFrames[i], *its);
				arrow->setType(IplImageItem::RECT);
				arrow->setFrameColor(QColor(255, 200, 0));
				float centerX = m_gridX[lostFrames[i] - m_firstFrame] + (float)m_gridWidth[lostFrames[i] - m_firstFrame] / 2;
				float centerY = m_gridY[objIndex] + (float)m_gridHeight[objIndex] / 2;
				arrow->setPos(centerX, centerY);
				arrow->show();
			}
			++objIndex;
		}	// for (its = m_selectedObjects.begin(); its != m_selectedObjects.end(); ++its, ++objIndex)
	}


	// 描画範囲の指定
	//QRectF visibleArea = mapToScene(rect()).boundingRect();
	QRectF visibleArea = mapToScene(viewport()->rect()).boundingRect();;
	QList<QRectF> rects;
	rects.append(visibleArea);

	// 描画をスケジュール
	//m_pStructData->mutex.lock();
	QGraphicsView::updateScene(rects);
	//m_pStructData->mutex.unlock();


	// 描画イメージ取得スレッドを停止する。
	while (m_pFaceImageGetter->isRunning())
	{
		m_pFaceImageGetter->stop();
		Sleep(0);
	}
	m_pFaceImageGetter->clearQue();

	// 画像取得リストを画像取得処理に渡す。
	std::map<int, std::vector<std::pair<SRegion::Type, int>>>::iterator itf, itb;
	itf = forwardItemList.begin();
	itb = backwardItemList.begin();
	//m_pStructData->mutex.lock();
	while (itf != forwardItemList.end() || itb != backwardItemList.end())
	{
		if (itf != forwardItemList.end())
		{
			m_pFaceImageGetter->addObjectImageQue(m_curFrame + itf->first, itf->second);
			++itf;
		}

		if (itb != backwardItemList.end())
		{
			m_pFaceImageGetter->addObjectImageQue(m_curFrame - itb->first, itb->second);
			++itb;
		}
	}
	//m_pStructData->mutex.unlock();

	m_pFaceImageGetter->start();
	busy = false;
}

void FaceGridView::onUpdateImage()
{
	//QRectF visibleArea = mapToScene(rect()).boundingRect();
	QRectF visibleArea = mapToScene(viewport()->rect()).boundingRect();
	QList<QRectF> rects;
	rects.append(visibleArea);

	m_pStructData->mutex.lock();
	QGraphicsView::updateScene(rects);
	m_pStructData->mutex.unlock();
}

void FaceGridView::mouseMoveEvent(QMouseEvent *event)
{
	//m_highLightRow = -1;
	//m_highLightCols.clear();

	QPointF scenePos = mapToScene(event->pos());
		
	if (m_bRightDrag)
	{
		QPointF newPos = event->pos();
		QMatrix mat = matrix();

		QPointF newCenter;
		newCenter.setX(m_dragCenter.x() + (m_grabPoint.x() - newPos.x()) / mat.m11());
		newCenter.setY(m_dragCenter.y() + (m_grabPoint.y() - newPos.y()) / mat.m11());

		m_pStructData->mutex.lock();
		centerOn(newCenter);
		m_pStructData->mutex.unlock();

		QList<QRectF> rects;
		//QRectF visibleArea = mapToScene(rect()).boundingRect();
		QRectF visibleArea = mapToScene(viewport()->rect()).boundingRect();
		rects.append(visibleArea);

		m_pStructData->mutex.lock();
		QGraphicsView::updateScene(rects);
		m_pStructData->mutex.unlock();

		return;
	}
/*	
	if (!m_bDragging)
	{
		m_dragMode = NONE;

		QList<QGraphicsItem*> items = this->items(event->pos());
		QGraphicsItem *item;
		m_targetItem = 0;
		QCursor cursor;
		
		foreach(item, items)
		{
			IplImageItem *face = (IplImageItem*)item;
			if (!face->isVisible())
				continue;
			if (!face->isSelected())
				continue;

			QPointF itemPoint = face->mapFromScene(scenePos);
			QRectF bounding = face->boundingRect();
			if (bounding.contains(itemPoint))
			{
				cursor.setShape(Qt::SizeAllCursor);
				m_dragMode = MOVE;
				m_grabPoint = itemPoint;
					
				m_targetItem = face;
				break;
			}	// if (bounding.contains(itemPoint))
		}	// foreach(item, items)

		if (!m_targetItem)
			unsetCursor();
		else
			setCursor(cursor);
	}
	else
	{
		if (m_dragMode == MOVE)
		{
			QPointF newPos = scenePos - m_grabPoint;
			QPointF delta = newPos - m_targetItem->scenePos();
			delta.setX(0);

			std::set<IplImageItem *>::iterator its;
			for (its = m_selectedItems.begin(); its != m_selectedItems.end(); ++its)
				(*its)->setPos((*its)->scenePos() + delta);

			m_highLightCols.clear();
			m_highLightRow = -1;
			if (scenePos.y() > 0)
			{
				for (int i = 0; i < m_gridY.size(); ++i)
				{
					if (scenePos.y() > m_gridY[i] && scenePos.y() < m_gridY[i] + m_gridHeight[i])
					{
						m_highLightRow = i;
						break;
					}
				}
				if (m_highLightRow >= 0)
				{
					for (its = m_selectedItems.begin(); its != m_selectedItems.end(); ++its)
						m_highLightCols.insert((*its)->getFrameID() - m_firstFrame);
				}
			}
			//m_highLights
			//m_targetItem->setPos(mapToScene(event->pos()) - m_grabPoint);

			QList<QRectF> rects;
			QRectF visibleArea = mapToScene(rect()).boundingRect();
			rects.append(visibleArea);
			QGraphicsView::updateScene(rects);
		}
	}	
*/	
	QGraphicsView::mouseMoveEvent(event);
}

void FaceGridView::mousePressEvent(QMouseEvent *event)
{
	if (event->buttons() == Qt::RightButton)
	{
		QList<QGraphicsItem *> itms;
		itms = items(event->pos());

		//if (itms.empty())
		{
			m_bRightDrag = true;
			QCursor cursor;
			cursor.setShape(Qt::ClosedHandCursor);
			setCursor(cursor);

			QPoint rectCenter = rect().center();
			rectCenter.setX(rectCenter.x() - 8);
			rectCenter.setY(rectCenter.y() - 8);
			m_dragCenter = mapToScene(rectCenter);

			m_grabPoint = event->pos();

			m_selectedRegion[0] = mapToScene(event->pos());
			return;
		}
	}
	else if (event->buttons() == Qt::MidButton)
	{
		setDragMode(QGraphicsView::RubberBandDrag);
		m_bMidDrag = true;
	}
	else if (event->buttons() == Qt::LeftButton)
	{
		setDragMode(QGraphicsView::RubberBandDrag);
	}

	m_selectedRegion[0] = mapToScene(event->pos());

	QGraphicsView::mousePressEvent(event);
}

void FaceGridView::mouseDoubleClickEvent(QMouseEvent *event)
{
	//qDebug() << "double clicked";
	m_bInDoubleClicking = true;

	while (m_pFaceImageGetter->isRunning())
	{
		m_pFaceImageGetter->stop();
	}

	QGraphicsView::mouseDoubleClickEvent(event);
}

void FaceGridView::mouseReleaseEvent(QMouseEvent *event)
{
	if (m_bInDoubleClicking)
	{
		m_bInDoubleClicking = false;
		return;
	}

	//qDebug() << event->buttons();

//void DlgFaceGrid::onItemClicked(int trackID, int frameID, int objectID, bool bCtrl, Qt::MouseButtons buttons)
//{

	int dragX = m_grabPoint.x() - event->pos().x();
	int dragY = m_grabPoint.y() - event->pos().y();

	if (true)//event->buttons() != Qt::RightButton)
	{
		QPointF pos = mapToScene(event->pos());

		if (pos.x() < m_selectedRegion[0].x())
		{
			m_selectedRegion[1].setX(m_selectedRegion[0].x());
			m_selectedRegion[0].setX(pos.x());
		}
		else
		{
			m_selectedRegion[1].setX(pos.x());
		}

		if (pos.y() < m_selectedRegion[0].y())
		{
			m_selectedRegion[1].setY(m_selectedRegion[0].y());
			m_selectedRegion[0].setY(pos.y());
		}
		else
		{
			m_selectedRegion[1].setY(pos.y());
		}

		int w = (m_selectedRegion[1].x() -  m_selectedRegion[0].x()) + 0.5;
		int h = (m_selectedRegion[1].y() -  m_selectedRegion[0].y()) + 0.5;

		if (m_bMidDrag)
		{
			if (w < 5 && h < 5)
			{
				float scale = adjustScale();
				emit sgScaleChanged(scale);
			}
			else
			{
				CvRect r;
				r.x = m_selectedRegion[0].x() + 0.5;
				r.y = m_selectedRegion[0].y() + 0.5;
				r.width = w;
				r.height = h;

				float scale = adjustScale(r.width, r.height);

				emit sgScaleChanged(scale);
				m_pStructData->mutex.lock();
				centerOn(r.x + r.width / 2, r.y + r.height / 2);
				m_pStructData->mutex.unlock();
			}
			m_bMidDrag = false;

			//setDragMode(QGraphicsView::NoDrag);

			QGraphicsView::mouseReleaseEvent(event);
			return;
		}
		else if ((m_bRightDrag && dragX < 5 && dragY < 5) || event->button() != Qt::RightButton && w < 5 && h < 5)
		{
			QList<QGraphicsItem *> itms;
			itms = items(event->pos());

			if (!itms.isEmpty())
			{
				IplImageItem *clickedItem =  qgraphicsitem_cast<IplImageItem*>(itms[0]);
				qDebug() << event->button();

				bool bCtrl = (event->modifiers() & Qt::ControlModifier);
				m_pParentDialog->onItemClicked(clickedItem->getTrackID(), clickedItem->getFrameID(), clickedItem->getObjectID(), bCtrl, event->button());
			}
		}
	
		if (!m_bRightDrag)
			selectFaces(event->modifiers());
	}

	if (m_bRightDrag)
	{
		m_bRightDrag = false;
		unsetCursor();
		//return;
	}

	QGraphicsView::mouseReleaseEvent(event);
}

void FaceGridView::selectFaces(Qt::KeyboardModifiers modifiers)//bool bCtrl)
{
	std::set<int> oldSelection = *m_pSelectedItems;

	std::vector<IplImageItem*> objects;
	int nearestItem = -1;

	objects.clear();

	// 顔領域のリスト作成
	QRect selection(m_selectedRegion[0].x(), m_selectedRegion[0].y(),
					m_selectedRegion[1].x() - m_selectedRegion[0].x(), m_selectedRegion[1].y() - m_selectedRegion[0].y());

	// 選択領域の幅、高さがともに3未満の場合は、点選択とする。
	bool bPoint = (selection.width() < 3 && selection.height() < 3);
	int cx = (m_selectedRegion[1].x() + m_selectedRegion[0].x()) / 2;
	int cy = (m_selectedRegion[1].y() + m_selectedRegion[0].y()) / 2;
	int minDist = -1, dx, dy;

	//QList<QGraphicsItem *> faceItems;
	//
	//if (m_detectType == Constants::TypeFace)
	//	faceItems = m_pFaceItems->childItems();
	//else if (m_detectType == Constants::TypePlate)
	//	faceItems = m_pPlateItems->childItems();
	
	//QGraphicsItem *faceItem;

	if ((modifiers & Qt::ControlModifier | modifiers & Qt::ShiftModifier) == 0)//(!bCtrl)
	{
		m_pSelectedItems->clear();
		*m_pLastSelected = -1;
	}

	for (int i = 0; i < static_cast<int>(SRegion::TypeNum); i++)
	{
		SRegion::Type targetType = static_cast<SRegion::Type>(i);
		std::map<SRegion::Type, QGraphicsItemGroup*>::iterator itTargetItemGroup = m_pObjectItems.find(targetType);
		if (itTargetItemGroup == m_pObjectItems.end())
			continue;
		foreach (QGraphicsItem *objectItem, itTargetItemGroup->second->childItems())
		{
			IplImageItem *object =  qgraphicsitem_cast<IplImageItem*>(objectItem);

			int fw = object->m_image.width();
			int fh = object->m_image.height();

			QRectF candidate(object->scenePos().x() - fw / 2, object->scenePos().y() - fh / 2, fw, fh);
			QPointF objectCenter = candidate.center();

			if ((!bPoint && candidate.intersects(selection)) || (bPoint && candidate.contains(cx, cy)))
			{
				objects.push_back(object);

				dx = cx - objectCenter.x();
				dy = cy - objectCenter.y();
				int dist = dx * dx + dy * dy;
				if (minDist < 0 || dist < minDist)
				{
					minDist = dist;
					//lastSelected = face;
					nearestItem = object->getTrackID();
				}
			}
		}
	}

	if ((modifiers & Qt::ShiftModifier) > 0)
	{
		std::set<int> objectSet;
		for (unsigned int i = 0; i < objects.size(); ++i)
			objectSet.insert(objects[i]->getObjectID());

		StructData::iterator<SRegion> itr;
		for (std::set<int>::iterator its = objectSet.begin(); its != objectSet.end(); ++its)
		{
			for (itr = m_pStructData->regionBegin(*its); itr != m_pStructData->regionEnd(); ++itr)
			{
				//if (itr->type != SRegion::TypeFace)
				//	continue;

				m_pSelectedItems->insert(itr.getID());
				*m_pLastSelected = itr.getID();
			}
		}
	}
	else
	{
		if (objects.size() == 1)
		{
			if (oldSelection.find(nearestItem) != oldSelection.end())
			{
				if ((modifiers & Qt::ControlModifier) == 0)//!bCtrl)
				{
					m_pSelectedItems->insert(nearestItem);
					*m_pLastSelected = nearestItem;
				}
				else
				{
					m_pSelectedItems->erase(nearestItem);
				}
			}
			else
			{
				m_pSelectedItems->insert(nearestItem);
				*m_pLastSelected = nearestItem;
			}
		}
		else
		{
			if (!bPoint)
			{
				for (unsigned int i = 0; i < objects.size(); ++i)
				{
					m_pSelectedItems->insert(objects[i]->getTrackID());
				}
			}
			else
			{
				m_pSelectedItems->insert(nearestItem);
			}
			*m_pLastSelected = nearestItem;
		}
	}

	UpdateInfo info;
	info.type = UpdateInfo::SelectionChanged;
	updateView(false, &info);

	emit sgSelectionChanged();
}

void FaceGridView::wheelEvent( QWheelEvent * event )
{
	emit sgWheelEvent(event);
}

void FaceGridView::setFrame(int frame, bool bUpdate)
{
	if (m_curFrame == frame && !bUpdate)
		return;

	m_curFrame = frame;

	QRectF visibleArea = viewport()->rect();

	QPoint viewCenter;
	viewCenter.setX(viewport()->rect().width() / 2);
	viewCenter.setY(viewport()->rect().height() / 2);

	updateView();

	QPointF sceneCenter = mapToScene(viewCenter);

	float centerX;
	if (frame - m_firstFrame >= m_gridX.size())
		centerX = sceneCenter.x();
	else
		centerX = m_gridX[frame - m_firstFrame] + (float)m_gridWidth[frame - m_firstFrame] / 2;

	//centerOn(centerX, centerY);
	m_pStructData->mutex.lock();
	centerOn(centerX, sceneCenter.y());
	m_pStructData->mutex.unlock();
}

void FaceGridView::exportSelectedImages(const QString &filename)
{
	QFileInfo fileInfo(filename);
	QString filePath = fileInfo.path();
	QString fileBase = fileInfo.baseName();
	QString fileExt = fileInfo.suffix();	

	// スレッドを停止する
	while (m_pFaceImageGetter->isRunning())
	{
		m_pFaceImageGetter->stop();
	}

	//QString saveFile = filePath + "/" + fileBase + QString("_%1").arg(frame, 4, 10, QLatin1Char('0')) + "." + fileExt;
	QString offsetStr = fileBase.section('_', -1);
	bool bOk;
	int fileIndex = offsetStr.toInt(&bOk);
	QString baseName = fileBase.section('_', 0, 0);

	std::set<int> effFrames;
	StructData::iterator<SRegion> itr;
	std::set<int>::iterator its;
	for (its = m_pSelectedItems->begin(); its != m_pSelectedItems->end(); ++its)
	{
		if (*its < 0)
			continue;
		//qDebug() << *its;
		itr = m_pStructData->regionFitAt(*its);
		effFrames.insert(itr->frame);
	}

	for (its = effFrames.begin(); its != effFrames.end(); ++its)
	{
		m_pStructData->mutex.lock();
		cv::Mat frameImage = m_pAviManager->getImage(*its);
		m_pStructData->mutex.unlock();

		for (itr = m_pStructData->regionBegin(m_camera, *its); itr != m_pStructData->regionEnd(); ++itr)
		{
			//qDebug() << itr.getID();

			if (m_pSelectedItems->find(itr.getID()) == m_pSelectedItems->end())
				continue;
			cv::Mat faceImage = frameImage(cv::Rect(itr->rect));
			QString saveFile = filePath + "/" + baseName + QString("_%1").arg(fileIndex++, 4, 10, QLatin1Char('0')) + "." + fileExt;
			cv::imwrite(saveFile.toStdString(), faceImage);
		}
	}
}

void FaceGridView::exportSamples(const QString &dirname, bool bPositive)
{
	// スレッドを停止する
	while (m_pFaceImageGetter->isRunning())
	{
		m_pFaceImageGetter->stop();
	}

	QString pnString;
	if (bPositive)
		pnString = "positive";
	else
		pnString = "negative";

	QDir dir(dirname);
	dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);

	QFileInfoList list = dir.entryInfoList();

	QString path = dir.absolutePath();
	QString ext = "png";

	int maxIndex(-1);
	for (int i = 0; i < list.size(); ++i)
	{
		QFileInfo fileInfo = list.at(i);

		QString filePath = fileInfo.path();
		QString fileBase = fileInfo.baseName();
		QString fileExt = fileInfo.suffix();

		if (fileExt != "png")
			continue;

		QString offsetStr = fileBase.section('_', -1);
		bool bOk;
		int fileIndex = offsetStr.toInt(&bOk);

		if (fileBase.indexOf(pnString) >= 0)
		{
			if (fileIndex > maxIndex)
			{
				maxIndex = fileIndex;
				path = filePath;
				ext = fileExt;
			}
		}
	}
	int fileOffset = maxIndex + 1;;

	std::set<int> effFrames;
	StructData::iterator<SRegion> itr;
	std::set<int>::iterator its;
	for (its = m_pSelectedItems->begin(); its != m_pSelectedItems->end(); ++its)
	{
		if (*its < 0)
			continue;
		//qDebug() << *its;
		itr = m_pStructData->regionFitAt(*its);
		effFrames.insert(itr->frame);
	}

	int width = m_pAviManager->getWidth();
	int height = m_pAviManager->getHeight();

	for (its = effFrames.begin(); its != effFrames.end(); ++its)
	{
		cv::Mat frameImage;// = m_pAviManager->getImage(*its);
		bool bFirst = true;
		for (itr = m_pStructData->regionBegin(m_camera, *its); itr != m_pStructData->regionEnd(); ++itr)
		{
			//qDebug() << itr.getID();

			if (m_pSelectedItems->find(itr.getID()) == m_pSelectedItems->end())
				continue;

			cv::Rect portrait;
			portrait.x = itr->rect.x - itr->rect.width / 2;
			portrait.width = itr->rect.width * 2;
			portrait.y = itr->rect.y - itr->rect.height / 2;
			portrait.height = itr->rect.height * 2;

			if (portrait.x < 0)
				continue;
			if (portrait.x + portrait.width >= width)
				continue;
			if (portrait.y < 0)
				continue;
			if (portrait.y + portrait.height >= height)
				continue;

			if (bFirst)
			{
				bFirst = false;
				m_pStructData->mutex.lock();
				frameImage = m_pAviManager->getImage(*its);
				m_pStructData->mutex.unlock();
			}

			cv::Mat faceImage = frameImage(portrait);
			QString saveFile = path + "/" + QString("%1_%2").arg(pnString).arg(fileOffset++, 6, 10, QLatin1Char('0')) + "." + ext;
			cv::imwrite(saveFile.toStdString(), faceImage);
			//cv::imwrite(UnicodeToMultiByte(saveFile.toStdWString()), faceImage);
		}
	}
}

void FaceGridView::exportSamplesHeadShoulder(const QString &dirname, bool bPositive)
{
	if (!bPositive)
	{
		exportSamplesHeadShoulderNegative(dirname);
		return;
	}

	// スレッドを停止する
	while (m_pFaceImageGetter->isRunning())
	{
		m_pFaceImageGetter->stop();
	}

	QString pnString;
	if (bPositive)
		pnString = "positive";
	else
		pnString = "negative";

	QDir dir(dirname);
	dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);

	QFileInfoList list = dir.entryInfoList();

	QString path = dir.absolutePath();
	QString ext = "png";

	int maxIndex(-1);
	for (int i = 0; i < list.size(); ++i)
	{
		QFileInfo fileInfo = list.at(i);

		QString filePath = fileInfo.path();
		QString fileBase = fileInfo.baseName();
		QString fileExt = fileInfo.suffix();

		if (fileExt != "png")
			continue;

		QString offsetStr = fileBase.section('_', -1);
		bool bOk;
		int fileIndex = offsetStr.toInt(&bOk);

		if (fileBase.indexOf(pnString) >= 0)
		{
			if (fileIndex > maxIndex)
			{
				maxIndex = fileIndex;
				path = filePath;
				ext = fileExt;
			}
		}
	}
	int fileOffset = maxIndex + 1;;

	std::set<int> effFrames;
	StructData::iterator<SRegion> itr;
	std::set<int>::iterator its;
	for (its = m_pSelectedItems->begin(); its != m_pSelectedItems->end(); ++its)
	{
		if (*its < 0)
			continue;
		//qDebug() << *its;
		itr = m_pStructData->regionFitAt(*its);
		effFrames.insert(itr->frame);
	}

	int width = m_pAviManager->getWidth();
	int height = m_pAviManager->getHeight();

	for (its = effFrames.begin(); its != effFrames.end(); ++its)
	{
		cv::Mat frameImage;// = m_pAviManager->getImage(*its);
		bool bFirst = true;
		for (itr = m_pStructData->regionBegin(m_camera, *its); itr != m_pStructData->regionEnd(); ++itr)
		{
			//qDebug() << itr.getID();

			if (m_pSelectedItems->find(itr.getID()) == m_pSelectedItems->end())
				continue;

			cv::Rect portrait;
			portrait.x = itr->rect.x - itr->rect.width * 1.2;
			portrait.width = itr->rect.width * 3.4;

			portrait.y = itr->rect.y - itr->rect.height * 0.5;
			portrait.height = portrait.width * 1.0;//1.33;

			if (portrait.x < 0)
				continue;
			if (portrait.x + portrait.width >= width)
				continue;
			if (portrait.y < 0)
				continue;
			if (portrait.y + portrait.height >= height)
				continue;

			if (bFirst)
			{
				bFirst = false;
				m_pStructData->mutex.lock();
				frameImage = m_pAviManager->getImage(*its);
				m_pStructData->mutex.unlock();
			}

			//cv::Mat faceImage = frameImage(portrait);
			cv::Mat grayFace;
			cv::cvtColor( frameImage(portrait), grayFace, CV_BGR2GRAY);
			cv::Mat resizeFace;
			cv::resize(grayFace, resizeFace, cv::Size(48, 48));//66));

			QString saveFile = path + "/" + QString("%1_%2").arg(pnString).arg(fileOffset++, 6, 10, QLatin1Char('0')) + "." + ext;
			cv::imwrite(saveFile.toStdString(), resizeFace);
		}
	}
}

void FaceGridView::exportSamplesHeadShoulderNegative(const QString &dirname)
{
	bool bPositive = false;

	// スレッドを停止する
	while (m_pFaceImageGetter->isRunning())
	{
		m_pFaceImageGetter->stop();
	}

	QString pnString;
	if (bPositive)
		pnString = "positive";
	else
		pnString = "negative";

	QDir dir(dirname);
	dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);

	QFileInfoList list = dir.entryInfoList();

	QString path = dir.absolutePath();
	QString ext = "png";

	int maxIndex(-1);
	for (int i = 0; i < list.size(); ++i)
	{
		QFileInfo fileInfo = list.at(i);

		QString filePath = fileInfo.path();
		QString fileBase = fileInfo.baseName();
		QString fileExt = fileInfo.suffix();

		if (fileExt != "png")
			continue;

		QString offsetStr = fileBase.section('_', -1);
		bool bOk;
		int fileIndex = offsetStr.toInt(&bOk);

		if (fileBase.indexOf(pnString) >= 0)
		{
			if (fileIndex > maxIndex)
			{
				maxIndex = fileIndex;
				path = filePath;
				ext = fileExt;
			}
		}
	}
	int fileOffset = maxIndex + 1;;

	std::set<int> effFrames;
	StructData::iterator<SRegion> itr, itrMask;
	std::set<int>::iterator its;
	for (its = m_pSelectedItems->begin(); its != m_pSelectedItems->end(); ++its)
	{
		if (*its < 0)
			continue;
		//qDebug() << *its;
		itr = m_pStructData->regionFitAt(*its);
		effFrames.insert(itr->frame);
	}

	int width = m_pAviManager->getWidth();
	int height = m_pAviManager->getHeight();

	int minWidth(-1), maxWidth(-1), minHeight(-1), maxHeight(-1);

	for (its = effFrames.begin(); its != effFrames.end(); ++its)
	{
		cv::Mat frameImage;// = m_pAviManager->getImage(*its);
		cv::Mat grayImage, cannyImage, maskImage(height, width, CV_8UC1, cv::Scalar(0));

		bool bFirst = true;
		for (itr = m_pStructData->regionBegin(m_camera, *its); itr != m_pStructData->regionEnd(); ++itr)
		{
			if (bFirst)
			{
				bFirst = false;
				m_pStructData->mutex.lock();
				frameImage = m_pAviManager->getImage(*its);
				m_pStructData->mutex.unlock();
				cv::cvtColor(frameImage, grayImage, CV_BGR2GRAY);

				// マスク画像作成
				for (itrMask = m_pStructData->regionBegin(m_camera, *its); itrMask != m_pStructData->regionEnd(); ++itrMask)
				{
					if (m_pSelectedItems->find(itrMask.getID()) == m_pSelectedItems->end())
						continue;

					cv::Rect portrait;
					portrait.x = itrMask->rect.x - itrMask->rect.width * 1.2;
					portrait.width = itrMask->rect.width * 3.4;

					portrait.y = itrMask->rect.y - itrMask->rect.height * 0.5;
					portrait.height = portrait.width * 1.0;

					if (minWidth < 0 || minWidth > portrait.width)
						minWidth = portrait.width;
					if (minHeight < 0 || minHeight > portrait.height)
						minHeight = portrait.height;

					if (maxWidth < 0 || maxWidth < portrait.width)
						maxWidth = portrait.width;
					if (maxHeight < 0 || maxHeight < portrait.height)
						maxHeight = portrait.height;

					cv::rectangle(maskImage, portrait, cv::Scalar(255), CV_FILLED);
				}

				cv::Canny(grayImage, cannyImage, 50, 150);

				//cv::namedWindow("mask");
				//cv::imshow("mask", maskImage);
				//cv::waitKey();
			}

			if (m_pSelectedItems->find(itr.getID()) == m_pSelectedItems->end())
				continue;

			cv::Rect portrait;
			portrait.x = itr->rect.x - itr->rect.width * 1.2;
			portrait.width = itr->rect.width * 3.4;

			portrait.y = itr->rect.y - itr->rect.height * 0.5;
			portrait.height = portrait.width * 1.0;//1.33;

			/*
			if (portrait.x < 0)
				continue;
			if (portrait.x + portrait.width >= width)
				continue;
			if (portrait.y < 0)
				continue;
			if (portrait.y + portrait.height >= height)
				continue;
			*/

			cv::Rect sampleRect(portrait);
			int effCount = 0;
			for (int j = 0; j < 1000; ++j)
			{
				sampleRect.x = rand() % width;
				sampleRect.y = rand() % height;

				if (sampleRect.x + sampleRect.width >= width)
					continue;
				if (sampleRect.y + sampleRect.height >= height)
					continue;

				int nonZero = cv::countNonZero(cannyImage(sampleRect));
				if (nonZero < 10)
					continue;

				int maskCount = cv::countNonZero(maskImage(sampleRect));
				if (maskCount > 10)
					continue;

				//cv::Mat grayFace;
				//cv::cvtColor( frameImage(portrait), grayFace, CV_BGR2GRAY);
				cv::Mat resizeSample;
				cv::resize(grayImage(sampleRect), resizeSample, cv::Size(48, 48));//66));

				QString saveFile = path + "/" + QString("%1_%2").arg(pnString).arg(fileOffset++, 6, 10, QLatin1Char('0')) + "." + ext;
				cv::imwrite(saveFile.toStdString(), resizeSample);

				// マスク追加
				cv::rectangle(maskImage, sampleRect, cv::Scalar(255), CV_FILLED);

				if (effCount++ > 1)
					break;
			}

/*
			cv::Mat grayFace;
			cv::cvtColor( frameImage(portrait), grayFace, CV_BGR2GRAY);
			cv::Mat resizeFace;
			cv::resize(grayFace, resizeFace, cv::Size(50, 50));//66));

			QString saveFile = path + "/" + QString("%1_%2").arg(pnString).arg(fileOffset++, 6, 10, QLatin1Char('0')) + "." + ext;
			cv::imwrite(saveFile.toStdString(), resizeFace);
*/
		}
	}

	qDebug() << "min/max size" << minWidth << minHeight << maxWidth << maxHeight;
}

void FaceGridView::setWaitRegion()
{
	m_bWaitRegion = true;
	QCursor cursor;
	cursor.setShape(Qt::WhatsThisCursor);
	setCursor(cursor);
}

void FaceGridView::unsetWaitRegion()
{
	m_bWaitRegion = false;
	unsetCursor();
}