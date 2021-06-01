#include "FrameFaceView.h"
#include "FrameFaceScene.h"
#include "RegionData.h"
#include "AviManager.h"
#include "IplImageItem.h"

FrameFaceView::FrameFaceView(QWidget *parent)
	: FaceView(parent)
{
}

FrameFaceView::~FrameFaceView()
{

}

void FrameFaceView::resizeEvent(QResizeEvent *event)
{
	alignFaces(event->size().width(), event->size().height());
}

void FrameFaceView::updateView()
{
	const QList<QGraphicsItem *> &items = scene()->items();
	for (unsigned int i = 0; i < items.size(); ++i)
		items[i]->hide();

	if (!isFrameValid(m_frame))
	{
		scene()->setSceneRect(0, 0, width(), height());
		return;
	}

	StructData::iterator<SRegion> itr;

	m_pStructData->mutex.lock();
	cv::Mat image = m_pAviManager->getImage(m_frame);
	m_pStructData->mutex.unlock();

	int faceCount = 0;
	for (itr = m_pStructData->regionBegin(m_camera, m_frame); itr != m_pStructData->regionEnd(); ++itr)
	{
		if (itr->type != SRegion::TypeFace)
			continue;
		
		++faceCount;
		if (scene()->items().size() < faceCount)
		{
			IplImageItem *newItem = new IplImageItem;
			connect(newItem, SIGNAL(sgItemClicked(int, bool, Qt::MouseButtons)), (QObject*)m_pParentDialog, SLOT(onItemClickedFaceView(int, bool, Qt::MouseButtons)));
			connect(newItem, SIGNAL(sgItemDoubleClicked(int)), (QObject*)m_pParentDialog, SIGNAL(sgItemDoubleClicked(int)));
			scene()->addItem(newItem);
		}
		
		IplImageItem *face = (IplImageItem*)(scene()->items()[faceCount - 1]);
		convIplToQImage(image, face->m_image, &(itr->rect));
		
		if (m_pStructData->checkRegionFlags(itr, SRegion::FlagTracked))
			face->setFrameColor(QColor(255, 0, 255));
		else
			face->setFrameColor(QColor(0, 255, 255));

		//face->setTrackID(itr.getID());
		face->setTrackIndices(itr.getID(), itr->frame, itr->object);
		face->setType(IplImageItem::IMAGE);
		face->show();
	}
	alignFaces();
}

void FrameFaceView::alignFaces(int w, int h)
{
	const QList<QGraphicsItem *> &items = scene()->items();
	unsigned int i = 0;
	std::vector<IplImageItem *> facesInRow;
	int maxRowWidth = 0;
	int prevBottom = 0;

	for (;;)
	{
		int rowWidth = 0;
		int maxHeight = 0;
		for (;;)
		{
			if (i >= items.size())
				break;
			if (!items[i]->isVisible())
				break;

			IplImageItem *face = (IplImageItem*)items[i];
			if (rowWidth + 2 * FaceMargin + face->m_image.width() * m_scale > width())
			{
				if (facesInRow.empty())
				{
					facesInRow.push_back(face);
					rowWidth += FaceMargin + face->m_image.width() * m_scale;
					maxHeight = face->m_image.height() * m_scale;
					++i;
				}
				break;
			}
			
			facesInRow.push_back(face);
			rowWidth += FaceMargin + face->m_image.width() * m_scale;
			if (face->m_image.height() * m_scale > maxHeight)
				maxHeight = face->m_image.height() * m_scale;
			++i;
		}

		if (facesInRow.empty())
			break;

		int baseLine = prevBottom + FaceMargin + maxHeight / 2;
		int prevRight = 0; 
		for (unsigned int j = 0; j < facesInRow.size(); ++j)
		{
			facesInRow[j]->setScale(m_scale);

			facesInRow[j]->setSelected(m_pSelectedItems->find(facesInRow[j]->getTrackID()) != m_pSelectedItems->end());

			facesInRow[j]->setPos(prevRight + FaceMargin + facesInRow[j]->m_image.width() * m_scale / 2, baseLine);
			prevRight += FaceMargin + facesInRow[j]->m_image.width() * m_scale;
		}
		prevBottom += FaceMargin + maxHeight;

		facesInRow.clear();

		if (rowWidth > maxRowWidth)
			maxRowWidth = rowWidth;
	}

	int oldWidth = w < 0 ? width() : w;
	int oldHeight = h < 0 ? height() : h;

	int newWidth = maxRowWidth + FaceMargin > oldWidth ? maxRowWidth + FaceMargin : oldWidth;
	int newHeight = prevBottom + FaceMargin > oldHeight ? prevBottom + FaceMargin : oldHeight;

	scene()->setSceneRect(0, 0, newWidth, newHeight);
	scene()->update();
}
