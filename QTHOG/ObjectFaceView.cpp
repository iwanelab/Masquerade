#include "ObjectFaceView.h"
#include "FrameFaceScene.h"
#include "RegionData.h"
#include "AviManager.h"
#include "IplImageItem.h"
#include "DlgFaces.h"
#include <QDebug>

ObjectFaceView::ObjectFaceView(QWidget *parent)
	: FaceView(parent),
	m_scale(1)
{

}

ObjectFaceView::~ObjectFaceView()
{
	m_cacheImages.clear();
	m_faceCacheImages.clear();
	//std::map<int, cv::Mat>::iterator it;
	//for (it = m_cacheImages.begin(); it != m_cacheImages.end(); ++it)
	//	cvReleaseImage(&it->second);
	//for (it = m_faceCacheImages.begin(); it != m_faceCacheImages.end(); ++it)
	//	cvReleaseImage(&it->second);
}

void ObjectFaceView::calcGrid()
{
	m_firstFrame = m_lastFrame = -1;
	m_selectedObjects.clear();
	std::set<int>::iterator its;
	StructData::iterator<SRegion> itr;

	for (its = m_pSelectedItems->begin(); its != m_pSelectedItems->end(); ++its)
	{
		itr = m_pStructData->regionFitAt(*its);
		m_selectedObjects.insert(itr->object);
	}

	m_gridX.clear();
	m_gridY.clear();
	m_gridWidth.clear();
	m_gridHeight.clear();

	int frameCount = m_pAviManager->getFrameCount();

	if (frameCount == 0)
		m_gridWidth.resize(1, FaceMargin);
	else
		m_gridWidth.resize(frameCount - 1, FaceMargin);

	for (its = m_selectedObjects.begin(); its != m_selectedObjects.end(); ++its)
	{
		if (m_gridY.empty())
			m_gridY.push_back(0);
		else
			m_gridY.push_back(m_gridY[m_gridY.size() - 1] + m_gridHeight.back());

		m_gridHeight.push_back(0);
		int &maxHeight = m_gridHeight.back();

		for (itr = m_pStructData->regionBegin(*its); itr != m_pStructData->regionEnd(); ++itr)
		{
			if (itr->type != SRegion::TypeFace)
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
}

void ObjectFaceView::drawBackground(QPainter *painter, const QRectF &rect)
{
	QPointF tl = mapToScene(0, 0);
	QPointF br = mapToScene(this->rect().width(), this->rect().height());

	//painter->fillRect(origin.x(), origin.y(), frameRect().width() * m_scale, frameRect().height() * m_scale/* + 500*/, QColor(70, 70, 70));
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

	QPen pen;
	pen.setColor(QColor(70, 70, 70));
	painter->setPen(pen);
	
	for (int i = 1; i < m_gridX.size(); ++i)
		painter->drawLine(m_gridX[i], 0, m_gridX[i], totalHeight);
}

void ObjectFaceView::setScale(float scale)
{
	QMatrix scaleMat(scale, 0, 0, scale, 0, 0);
	setMatrix(scaleMat);
	m_scale = scale;
}

void ObjectFaceView::updateView(bool bCenterOn)
{
	int frameCount = m_pAviManager->getFrameCount();

	calcGrid();

	if (m_selectedObjects.empty())
	{
		scene()->setSceneRect(0, 0, FaceMargin, FaceMargin);
		m_pParentDialog->setObjectViewSize(0);
		return;
	}

	int totalWidth = m_gridX.back() + m_gridWidth.back();
	int totalHeight = m_gridY.back() + m_gridHeight.back();

	//qDebug() << "totalHeight" << totalHeight;

	//scene()->setSceneRect(0, 0, totalWidth, totalHeight);
	int firstFrame = m_firstFrame > 0 ? m_firstFrame - 1 : 0;
	int lastFrame = m_lastFrame < m_gridX.size() - 1 ? m_lastFrame + 1 : m_gridX.size() - 1;
	scene()->setSceneRect(m_gridX[firstFrame], 0, m_gridX[lastFrame] + m_gridWidth[lastFrame] - m_gridX[firstFrame], totalHeight);
	
	m_pParentDialog->setObjectViewSize(totalHeight * m_scale + 80);// > height() / 2 ? height() / 2 : totalHeight);

	//m_pParentDialog->setObjectViewSize(100);

	const QList<QGraphicsItem *> &items = scene()->items();
	for (unsigned int i = 0; i < items.size(); ++i)
		items[i]->hide();

	std::set<int>::iterator its;
	StructData::iterator<SRegion> itr;

	int itemCount = 0;
	int objIndex = 0;
	for (its = m_selectedObjects.begin(); its != m_selectedObjects.end(); ++its, ++objIndex)
	{
		int firstFrame = -1, lastFrame = -1;
		std::vector<int> lostFrames;

		for (itr = m_pStructData->regionBegin(*its); itr != m_pStructData->regionEnd(); ++itr)
		{
			if (itr->type != SRegion::TypeFace)
				continue;

			int frame = itr->frame;

			if (firstFrame < 0)
				firstFrame = frame;
			else
			{
				for (int lostFrame = lastFrame + 1; lostFrame < frame; ++lostFrame)
					lostFrames.push_back(lostFrame);
			}

			lastFrame = frame;

			++itemCount;
			if (scene()->items().size() < itemCount)
			{
				IplImageItem *newItem = new IplImageItem;
				newItem->setCenterOnClicked(false);
				connect(newItem, SIGNAL(sgItemClicked(int, bool, Qt::MouseButtons)), m_pParentDialog, SLOT(onItemClickedObjectView(int, bool, Qt::MouseButtons)));
				connect(newItem, SIGNAL(sgItemDoubleClicked(int)), m_pParentDialog, SIGNAL(sgItemDoubleClicked(int)));
				connect(newItem, SIGNAL(sgItemDoubleClicked(int, int, bool)), m_pParentDialog, SIGNAL(sgItemDoubleClicked(int, int, bool)));
				scene()->addItem(newItem);
			}
			
			IplImageItem *face = (IplImageItem*)(scene()->items()[itemCount - 1]);

			// 顔画像のキャッシュを使うテスト
			if (m_faceCacheImages.find(itr.getID()) == m_faceCacheImages.end())
			{
				// フレーム画像キャッシュの利用
				if (m_cacheImages.find(frame) == m_cacheImages.end())
				{
					m_pStructData->mutex.lock();
					m_cacheImages[frame] = m_pAviManager->getImage(frame);
					m_pStructData->mutex.unlock();
					m_cacheFrames.push_back(frame);

					if (m_cacheFrames.size() > CacheSize)
					{
						int eraseFrame = *(m_cacheFrames.begin());
						m_cacheImages.erase(eraseFrame);
						m_cacheFrames.pop_front();
					}
				}

				m_faceCacheImages[itr.getID()] = m_cacheImages[frame](itr->rect);
				//cvSetImageROI(m_cacheImages[frame], itr->rect);
				////m_faceCacheImages[itr.getID()] = cvCloneImage(m_cacheImages[frame]);
				//m_faceCacheImages[itr.getID()] = cvCreateImage(cvSize(itr->rect.width, itr->rect.height), IPL_DEPTH_8U, 3);
				//cvCopy(m_cacheImages[frame], m_faceCacheImages[itr.getID()]);
				//cvResetImageROI(m_cacheImages[frame]);
				m_faceCacheRects[itr.getID()] = itr->rect;

				m_cacheFaces.push_back(itr.getID());

				if (m_cacheFaces.size() > FaceCacheSize)
				{
					int eraseFace = *(m_cacheFaces.begin());
					m_faceCacheImages.erase(eraseFace);
					m_faceCacheRects.erase(eraseFace);
					m_cacheFaces.pop_front();
				}
			}
			else
			{
				// 顔画像キャッシュがあったが、念のためサイズを比較
				CvRect &r = m_faceCacheRects[itr.getID()];
				if (itr->rect.x != r.x || itr->rect.y != r.y || itr->rect.width != r.width || itr->rect.height != r.height)
				{
					//qDebug() << "cached rect obsolated!";

					// フレーム画像キャッシュの利用
					if (m_cacheImages.find(frame) == m_cacheImages.end())
					{
						m_pStructData->mutex.lock();
						m_cacheImages[frame] = m_pAviManager->getImage(frame);
						m_pStructData->mutex.unlock();
						m_cacheFrames.push_back(frame);

						if (m_cacheFrames.size() > CacheSize)
						{
							int eraseFrame = *(m_cacheFrames.begin());
							m_cacheImages.erase(eraseFrame);
							m_cacheFrames.pop_front();
						}
					}

					m_faceCacheImages[itr.getID()] = m_cacheImages[frame](itr->rect);
					//cvSetImageROI(m_cacheImages[frame], itr->rect);
					////m_faceCacheImages[itr.getID()] = cvCloneImage(m_cacheImages[frame]);
					//m_faceCacheImages[itr.getID()] = cvCreateImage(cvSize(itr->rect.width, itr->rect.height), IPL_DEPTH_8U, 3);
					//cvCopy(m_cacheImages[frame], m_faceCacheImages[itr.getID()]);
					//cvResetImageROI(m_cacheImages[frame]);
					m_faceCacheRects[itr.getID()] = itr->rect;
				}
			}

			//convIplToQImage(m_cacheImages[frame], face->m_image, &(itr->rect));
			convIplToQImage(m_faceCacheImages[itr.getID()], face->m_image);
			
			if (m_pStructData->checkRegionFlags(itr, SRegion::FlagTracked))
				face->setFrameColor(QColor(255, 0, 255));
			else
				face->setFrameColor(QColor(0, 255, 255));

			face->setSelected(m_pSelectedItems->find(itr.getID()) != m_pSelectedItems->end());
			float centerX = m_gridX[frame] + (float)m_gridWidth[frame] / 2;
			float centerY = m_gridY[objIndex] + (float)m_gridHeight[objIndex] / 2;

			face->setPos(centerX, centerY);
			//face->setTrackID(itr.getID());
			face->setTrackIndices(itr.getID(), itr->frame, itr->object);
			face->setType(IplImageItem::IMAGE);
			face->show();

			if (bCenterOn && *m_pLastSelected >= 0 && *m_pLastSelected == itr.getID())
				centerOn(centerX, centerY);
		}	// for (itr = m_pStructData->regionBegin(*its); itr != m_pStructData->regionEnd(); ++itr)

		if (firstFrame > 0)
		{
			++itemCount;
			if (scene()->items().size() < itemCount)
			{
				IplImageItem *newItem = new IplImageItem;
				newItem->setCenterOnClicked(false);
				connect(newItem, SIGNAL(sgItemClicked(int, bool, Qt::MouseButtons)), m_pParentDialog, SLOT(onItemClickedObjectView(int, bool, Qt::MouseButtons)));
				connect(newItem, SIGNAL(sgItemDoubleClicked(int)), m_pParentDialog, SIGNAL(sgItemDoubleClicked(int)));
				connect(newItem, SIGNAL(sgItemDoubleClicked(int, int, bool)), m_pParentDialog, SIGNAL(sgItemDoubleClicked(int, int, bool)));
				scene()->addItem(newItem);
			}
			IplImageItem *face = (IplImageItem*)(scene()->items()[itemCount - 1]);

			face->setTrackIndices(-1, firstFrame - 1, *its);
			face->setType(IplImageItem::LARROW);
			face->setFrameColor(QColor(0, 255, 0));
			float centerX = m_gridX[firstFrame - 1] + (float)m_gridWidth[firstFrame - 1] / 2;
			float centerY = m_gridY[objIndex] + (float)m_gridHeight[objIndex] / 2;
			face->setPos(centerX, centerY);
			face->show();
		}
		if (lastFrame < frameCount - 1)
		{
			++itemCount;
			if (scene()->items().size() < itemCount)
			{
				IplImageItem *newItem = new IplImageItem;
				newItem->setCenterOnClicked(false);
				connect(newItem, SIGNAL(sgItemClicked(int, bool, Qt::MouseButtons)), m_pParentDialog, SLOT(onItemClickedObjectView(int, bool, Qt::MouseButtons)));
				connect(newItem, SIGNAL(sgItemDoubleClicked(int)), m_pParentDialog, SIGNAL(sgItemDoubleClicked(int)));
				connect(newItem, SIGNAL(sgItemDoubleClicked(int, int, bool)), m_pParentDialog, SIGNAL(sgItemDoubleClicked(int, int, bool)));
				scene()->addItem(newItem);
			}
			IplImageItem *face = (IplImageItem*)(scene()->items()[itemCount - 1]);

			face->setTrackIndices(-1, lastFrame + 1, *its);
			face->setType(IplImageItem::RARROW);
			face->setFrameColor(QColor(0, 255, 0));
			float centerX = m_gridX[lastFrame + 1] + (float)m_gridWidth[lastFrame + 1] / 2;
			float centerY = m_gridY[objIndex] + (float)m_gridHeight[objIndex] / 2;
			face->setPos(centerX, centerY);
			face->show();
		}
		for (int i = 0; i < lostFrames.size(); ++i)
		{
			++itemCount;
			if (scene()->items().size() < itemCount)
			{
				IplImageItem *newItem = new IplImageItem;
				newItem->setCenterOnClicked(false);
				connect(newItem, SIGNAL(sgItemClicked(int, bool, Qt::MouseButtons)), m_pParentDialog, SLOT(onItemClickedObjectView(int, bool, Qt::MouseButtons)));
				connect(newItem, SIGNAL(sgItemDoubleClicked(int)), m_pParentDialog, SIGNAL(sgItemDoubleClicked(int)));
				connect(newItem, SIGNAL(sgItemDoubleClicked(int, int, bool)), m_pParentDialog, SIGNAL(sgItemDoubleClicked(int, int, bool)));
				scene()->addItem(newItem);
			}
			IplImageItem *face = (IplImageItem*)(scene()->items()[itemCount - 1]);

			face->setTrackIndices(-1, lostFrames[i], *its);
			face->setType(IplImageItem::RECT);
			face->setFrameColor(QColor(255, 200, 0));
			float centerX = m_gridX[lostFrames[i]] + (float)m_gridWidth[lostFrames[i]] / 2;
			float centerY = m_gridY[objIndex] + (float)m_gridHeight[objIndex] / 2;
			face->setPos(centerX, centerY);
			face->show();
		}
	}	// for (its = m_selectedObjects.begin(); its != m_selectedObjects.end(); ++its, ++objIndex)
	
	QRectF visibleArea = mapToScene(rect()).boundingRect();
	QList<QRectF> rects;
	rects.append(visibleArea);
	QGraphicsView::updateScene(rects);
}
