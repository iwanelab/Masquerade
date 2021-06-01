#include "MergeView.h"
#include "IplImageItem.h"

MergeView::MergeView(QWidget *parent)
	: FaceView(parent),
	m_scale(1),
	m_bMidDrag(false),
	m_bRightDrag(false),
	m_bDragging(false),
	m_dragMode(NONE),
	m_highLightRow(-1)
{
	setInteractive(true);
	setDragMode(QGraphicsView::RubberBandDrag);
	setMouseTracking(true);
}

MergeView::~MergeView()
{
	if (m_pStructData)
		m_pStructData->release();
}

void MergeView::calcGrid()
{
	std::map<int, int> gridWidth;
	std::map<int, int> gridHeight;
	std::set<int> objects;

	std::map<int, std::vector<IplImageItem*> >::iterator it;

	m_mapObjectsToIndex.clear();
	m_mapIndexToObjects.clear();
	m_firstFrame = -1;
	m_lastFrame = -1;

	QList<QGraphicsItem *> faceItems = scene()->items();
	QGraphicsItem *faceItem;

	foreach (faceItem, faceItems)
	{
		IplImageItem *pItem = qgraphicsitem_cast<IplImageItem*>(faceItem);

			//IplImageItem* pItem = it->second[i];
			int frame = pItem->getFrameID();
			int object = pItem->getObjectID();

		if (object < 0)
			continue;

			if (gridHeight.find(object) == gridHeight.end() || gridHeight[object] < pItem->m_image.height() + FaceMargin)
				gridHeight[object] = pItem->m_image.height() + FaceMargin;

			if (gridWidth.find(frame) == gridWidth.end() || gridWidth[frame] < pItem->m_image.width() + FaceMargin)
				gridWidth[frame] = pItem->m_image.width() + FaceMargin;

			if (m_firstFrame < 0 || frame < m_firstFrame)
				m_firstFrame = frame;
			if (m_lastFrame < 0 || frame > m_lastFrame)
				m_lastFrame = frame;

			objects.insert(object);
		//}
	}

	m_gridX.resize(1, 0);
	m_gridY.resize(1, 0);
	m_gridWidth.clear();
	m_gridHeight.clear();

	std::set<int>::iterator its;
	for (its = objects.begin(); its != objects.end(); ++its)
	{
		//m_objects.push_back(*its);
		m_mapObjectsToIndex[*its] = m_mapObjectsToIndex.size();
		m_mapIndexToObjects[m_mapIndexToObjects.size()] = *its;

		m_gridHeight.push_back(gridHeight[*its]);
		m_gridY.push_back(m_gridY.back() + gridHeight[*its]);
	}
	m_gridY.pop_back();

	for (int frame = m_firstFrame; frame <= m_lastFrame; ++frame)
	{
		if (gridWidth.find(frame) == gridWidth.end())
			m_gridWidth.push_back(FaceMargin);
		else
			m_gridWidth.push_back(gridWidth[frame]);

		//m_gridWidth.push_back(m_gridWidth.back());
		if (frame < m_lastFrame)
			m_gridX.push_back(m_gridX.back() + m_gridWidth.back());
	}
}

void MergeView::drawBackground(QPainter *painter, const QRectF &rect)
{
	QPointF tl = mapToScene(0, 0);
	QPointF br = mapToScene(this->rect().width(), this->rect().height());
	painter->fillRect(tl.x(), tl.y(), br.x() - tl.x(), br.y() - tl.y(), QColor(70, 70, 70));

	if (m_gridHeight.empty())
		return;

	int totalWidth = m_gridX.back() + m_gridWidth.back();
	int totalHeight = m_gridY.back() + m_gridHeight.back();

	//int prevBottom = 0;
	int gray = 75;
	for (int i = 0; i < m_gridHeight.size(); ++i)
	{
		painter->fillRect(0, m_gridY[i]/*prevBottom*/, totalWidth, m_gridHeight[i], QColor(gray, gray, gray));
		//prevBottom += m_gridHeight[i];
		gray = (gray == 75) ? 80 : 75;
	}

	if (m_highLightRow >= 0)
	{
		for (std::set<int>::iterator it = m_highLightCols.begin(); it != m_highLightCols.end(); ++it)
			painter->fillRect(m_gridX[*it], m_gridY[m_highLightRow], m_gridWidth[*it], m_gridHeight[m_highLightRow], QColor(120, 120, 120));
	}


	QPen pen;
	pen.setColor(QColor(70, 70, 70));
	painter->setPen(pen);
	
	for (int i = 1; i < m_gridX.size(); ++i)
		painter->drawLine(m_gridX[i], 0, m_gridX[i], totalHeight);
}

void MergeView::setScale(float scale)
{
	QMatrix scaleMat(scale, 0, 0, scale, 0, 0);
	setMatrix(scaleMat);
	m_scale = scale;
}

void MergeView::addImageItem(IplImageItem *item)
{
	IplImageItem *newItem = new IplImageItem;
	*newItem = *item;

	scene()->addItem(newItem);
}

void MergeView::showEvent(QShowEvent *event)
{
	updateView();
}

void MergeView::updateView()
{
	calcGrid();

	if (m_gridHeight.empty())
	{
		scene()->setSceneRect(0, 0, FaceMargin, FaceMargin);
		return;
	}
	int totalWidth = m_gridX.back() + m_gridWidth.back();
	int totalHeight = m_gridY.back() + m_gridHeight.back();

	scene()->setSceneRect(0, 0, totalWidth, totalHeight);

	QList<QGraphicsItem *> faceItems = scene()->items();
	QGraphicsItem *faceItem;

	foreach (faceItem, faceItems)
	{
		IplImageItem *face =  qgraphicsitem_cast<IplImageItem*>(faceItem);

		if (face->getObjectID() < 0)
		{
			face->hide();
			continue;
		}

		int frameIndex = face->getFrameID() - m_firstFrame;
		int objectIndex = m_mapObjectsToIndex[face->getObjectID()];

		float centerX = m_gridX[frameIndex] + (float)m_gridWidth[frameIndex] / 2;
		float centerY = m_gridY[objectIndex] + (float)m_gridHeight[objectIndex] / 2;
		
		face->setPos(centerX, centerY);
		face->show();
	}

	QList<QRectF> rects;
	QRectF visibleArea = mapToScene(rect()).boundingRect();
	rects.append(visibleArea);
	QGraphicsView::updateScene(rects);
}

void MergeView::mouseMoveEvent(QMouseEvent *event)
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

		centerOn(newCenter);

		QList<QRectF> rects;
		QRectF visibleArea = mapToScene(rect()).boundingRect();
		rects.append(visibleArea);
		QGraphicsView::updateScene(rects);

		return;
	}
	
	if (!m_bDragging)
	{
		m_dragMode = NONE;

		QList<QGraphicsItem*> items = this->items(event->pos());
		QGraphicsItem *item;
		m_targetItem = 0;
		QCursor cursor;
		
		foreach(item, items)
		{
			IplImageItem *face =  qgraphicsitem_cast<IplImageItem*>(item);
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
	
	QGraphicsView::mouseMoveEvent(event);
}

void MergeView::mousePressEvent(QMouseEvent *event)
{
	if (event->buttons() == Qt::RightButton)
	{
		QList<QGraphicsItem *> itms;
		itms = items(event->pos());

		if (itms.empty())
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
			return;
		}
	}

	if (event->buttons() == Qt::MidButton)
		m_bMidDrag = true;

	if (event->buttons() == Qt::LeftButton)
	{
		if (!m_bDragging && m_dragMode != NONE)
		{
			m_bDragging = true;
			//mouseEvent->accept();
			//qDebug() << "press accept";

			int object = m_targetItem->getObjectID();
			std::set<IplImageItem *>::iterator its, itNext;
			for (its = m_selectedItems.begin(); its != m_selectedItems.end();)
			{
				itNext = its;
				++itNext;
				if ((*its)->getObjectID() != object)
				{
					(*its)->setSelected(false);
					m_selectedItems.erase(*its);
				}
				its = itNext;
			}
			return;
		}
	}

	m_selectedRegion[0] = mapToScene(event->pos());

	QGraphicsView::mousePressEvent(event);
}

void MergeView::mouseReleaseEvent(QMouseEvent *event)
{
	if (m_bRightDrag)
	{
		m_bRightDrag = false;
		unsetCursor();
		return;
	}

	if (m_bDragging && m_dragMode != NONE)
	{
		m_bDragging = false;

		if (m_highLightRow < 0)
		{
			m_highLightRow = -1;
			updateView();
			return;
		}

		int targetObject = m_mapIndexToObjects[m_highLightRow];
		int sourceObject = m_targetItem->getObjectID();

		if ((sourceObject == targetObject) ||
			!checkObjectType(sourceObject, targetObject))
		{
			m_highLightRow = -1;
			updateView();
			return;
		}

		// IDの付け替え
		QList<QGraphicsItem *> faceItems = scene()->items();
		QGraphicsItem *faceItem;

		foreach (faceItem, faceItems)
		{
			IplImageItem *face = qgraphicsitem_cast<IplImageItem*>(faceItem);

			if (face->getObjectID() < 0)
			{
				face->hide();
				continue;
			}
			
			if (face->getObjectID() == targetObject)
			{
				if (m_highLightCols.find(face->getFrameID() - m_firstFrame) != m_highLightCols.end())
					face->setObjectID(-1);
			}

			if (face->getObjectID() == sourceObject)
			{
				if (m_highLightCols.find(face->getFrameID() - m_firstFrame) != m_highLightCols.end())
					face->setObjectID(targetObject);
			}
		}
		
		m_highLightRow = -1;

		updateView();
		return;
	}

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
			centerOn(r.x + r.width / 2, r.y + r.height / 2);
		}
		m_bMidDrag = false;

		QGraphicsView::mouseReleaseEvent(event);
		return;
	}

	selectFaces(event->modifiers() & Qt::ControlModifier);

	QGraphicsView::mouseReleaseEvent(event);
}

void MergeView::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Delete)
	{
		std::set<IplImageItem*>::iterator it;
		for (it = m_selectedItems.begin(); it != m_selectedItems.end(); ++it)
			(*it)->setObjectID(-1);
	}
	updateView();
}

void MergeView::selectFaces(bool bCtrl)
{
	if (!bCtrl)
		m_selectedItems.clear();

	std::vector<IplImageItem*> faces;
	IplImageItem* lastSelected = 0;

	faces.clear();

	// 顔領域のリスト作成
	QRect selection(m_selectedRegion[0].x(), m_selectedRegion[0].y(),
					m_selectedRegion[1].x() - m_selectedRegion[0].x(), m_selectedRegion[1].y() - m_selectedRegion[0].y());

	bool bPoint = (selection.width() < 3 && selection.height() < 3);
	int cx = (m_selectedRegion[1].x() + m_selectedRegion[0].x()) / 2;
	int cy = (m_selectedRegion[1].y() + m_selectedRegion[0].y()) / 2;
	int minDist = -1, dx, dy;

	//for (it = m_pStructData->regionBegin(0, frame); it != m_pStructData->regionEnd(); ++it)
	//{
	QList<QGraphicsItem *> faceItems = scene()->items();
	QGraphicsItem *faceItem;

	foreach (faceItem, faceItems)
	{
		IplImageItem *face =  qgraphicsitem_cast<IplImageItem*>(faceItem);

		if (!face->isVisible())
			continue;

		if (!bCtrl)
			face->setSelected(false);

		int fw = face->m_image.width();
		int fh = face->m_image.height();

		QRectF candidate(face->scenePos().x() - fw / 2, face->scenePos().y() - fh / 2, fw, fh);
		QPointF faceCenter = candidate.center();

		if ((!bPoint && candidate.intersects(selection)) || (bPoint && candidate.contains(cx, cy)))
		{
			faces.push_back(face);

			dx = cx - faceCenter.x();
			dy = cy - faceCenter.y();
			int dist = dx * dx + dy * dy;
			if (minDist < 0 || dist < minDist)
			{
				minDist = dist;
				lastSelected = face;
			}
		}
	}

	if (lastSelected)
	{
		if (faces.size() == 1)
		{
			if (m_selectedItems.find(lastSelected) != m_selectedItems.end())
			{
				if (!bCtrl)
				{
					m_selectedItems.insert(lastSelected);
					lastSelected->setSelected(true);
				}
				else
				{
					m_selectedItems.erase(lastSelected);
					lastSelected->setSelected(false);
				}
			}
			else
			{
				m_selectedItems.insert(lastSelected);
				lastSelected->setSelected(true);
			}
		}
		else
		{
			if (!bPoint)
			{
				for (unsigned int i = 0; i < faces.size(); ++i)
				{
					m_selectedItems.insert(faces[i]);
					faces[i]->setSelected(true);
				}
			}
			else
			{
				m_selectedItems.insert(lastSelected);
				lastSelected->setSelected(true);
			}
		}
	}

	QList<QRectF> rects;
	QRectF visibleArea = mapToScene(rect()).boundingRect();
	rects.append(visibleArea);
	QGraphicsView::updateScene(rects);
}

void MergeView::getMapTrackToObject(std::map<int, int>& t2o)
{
	QList<QGraphicsItem *> faceItems = scene()->items();
	QGraphicsItem *faceItem;

	t2o.clear();
	foreach (faceItem, faceItems)
	{
		IplImageItem *face =  qgraphicsitem_cast<IplImageItem*>(faceItem);
		t2o[face->getTrackID()] = face->getObjectID();
	}
}

void MergeView::pack()
{
	std::map<int, std::vector<std::pair<int, IplImageItem*>>> items;

	QList<QGraphicsItem *> faceItems = scene()->items();
	QGraphicsItem *faceItem;
	foreach (faceItem, faceItems)
	{
		IplImageItem *face =  qgraphicsitem_cast<IplImageItem*>(faceItem);
		int frame = face->getFrameID();

		items[frame].push_back(std::pair<int, IplImageItem*>(face->getObjectID(), face));
	}

	// ID の付け替え
	std::map<int, std::vector<std::pair<int, IplImageItem*>>>::iterator it;
	for (it = items.begin(); it != items.end(); ++it)
	{
		std::sort(it->second.begin(), it->second.end());
		for (int i = 0; i < it->second.size(); ++i)
		{
			if (i == 0)
				it->second[i].second->setObjectID(m_mapIndexToObjects[i]);
			else
				it->second[i].second->setObjectID(-1);	// ２行目以下を削除
		}	
	}
/*
	if (event->key() == Qt::Key_Delete)
	{
		std::set<IplImageItem*>::iterator it;
		for (it = m_selectedItems.begin(); it != m_selectedItems.end(); ++it)
			(*it)->setObjectID(-1);
	}
*/
	updateView();
}
bool MergeView::checkObjectType(int targetObjectId, int sourceObjectId)
{
	std::set<SRegion::Type> objectType;
	StructData::iterator<SRegion> itr;

	for (itr = m_pStructData->regionBegin(targetObjectId); itr != m_pStructData->regionEnd(); ++itr)
	{
		if (itr->type != SRegion::TypeManual)
		{
			objectType.insert(itr->type);
			if (objectType.size() > 1)
				return false;
		}
	}
	for (itr = m_pStructData->regionBegin(sourceObjectId); itr != m_pStructData->regionEnd(); ++itr)
	{
		if (itr->type != SRegion::TypeManual)
		{
			objectType.insert(itr->type);
			if (objectType.size() > 1)
				return false;
		}
	}

	return true;
}
