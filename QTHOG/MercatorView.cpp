#include "MercatorView.h"
#include "RegionData.h"
#include "RectItem.h"
#include "ObjectColor.h"
#include <QGraphicsItem>
#include <QMouseEvent>
#include <QDebug>

MercatorView::MercatorView(QWidget *parent)
	: IplImageView(parent),
	//m_detectType(Constants::TypeFace),
	m_curFrame(0),
	m_curCamera(0),
	m_pStructData(0),
	m_numViewRegion(0)
{
	connect(scene(), SIGNAL(sgItemMoved(int, QPointF)), this, SLOT(onItemMoved(int, QPointF)));
	connect(scene(), SIGNAL(sgItemResized(int, QRectF)), this, SLOT(onItemResized(int, QRectF)));

	for (int typeNo = 0; typeNo < SRegion::TypeNum; typeNo++)
	{
		m_pItemGroup[static_cast<SRegion::Type>(typeNo)] = new QGraphicsItemGroup();
		m_pItemGroup[static_cast<SRegion::Type>(typeNo)]->setHandlesChildEvents(false);
		scene()->addItem(m_pItemGroup[static_cast<SRegion::Type>(typeNo)]);
		m_pItemGroup[static_cast<SRegion::Type>(typeNo)]->show();
	}
}

MercatorView::~MercatorView()
{
	if (m_pStructData)
		m_pStructData->release();
}

void MercatorView::onItemMoved(int id, QPointF pos)
{
	StructData::iterator<SRegion> itr = m_pStructData->regionFitAt(id);
	itr->rect.x = pos.x();
	itr->rect.y = pos.y();

	emit sgItemChanged();
	std::vector<int> changedImages(1, itr.getID());
	emit sgImageChanged(changedImages, true);
}

void MercatorView::onItemResized(int id, QRectF newRect)
{
	StructData::iterator<SRegion> itr = m_pStructData->regionFitAt(id);
	itr->rect.x = newRect.x() + 0.5;
	itr->rect.y = newRect.y() + 0.5;
	itr->rect.width = newRect.width() + 0.5;
	itr->rect.height = newRect.height() + 0.5;
	
	emit sgItemChanged();
	std::vector<int> changedImages(1, itr.getID());
	emit sgImageChanged(changedImages, true);
}

void MercatorView::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Delete)
	{
		std::set<int> deleteSet = *m_pSelectedItems;

		std::set<int>::iterator its;
		for (its = m_pSelectedItems->begin(); its != m_pSelectedItems->end(); ++its)
			m_pStructData->erase(m_pStructData->regionFitAt(*its));

		m_pSelectedItems->clear();
		*m_pLastSelected = -1;

		emit sgItemDeleted(deleteSet);
	}
	else
		IplImageView::keyPressEvent(event);
}

void MercatorView::drawBackground(QPainter *painter, const QRectF &rect)
{
	//m_mutex.lock();
	//bBusy = true;
	IplImageView::drawBackground(painter, rect);

	//bBusy = false;

	QPointF points[2];
	QPen pen;
	pen.setWidth(2);

	for (unsigned int i = 0; i < m_rects.size(); ++i)
	{
		pen.setColor(QColor(m_rects[i].r, m_rects[i].g, m_rects[i].b));
		painter->setPen(pen);
		painter->drawRect(QRectF(m_rects[i].rect.x, m_rects[i].rect.y, m_rects[i].rect.width, m_rects[i].rect.height));
	}

	//viewport()->setMouseTracking(true);
	//m_mutex.unlock();
}

void MercatorView::setFrame(int frame)
{	
	//m_mutex.lock();

	if (frame < 0)
		frame = m_curFrame;

	if (!m_pStructData)
		return;
	//if (m_pStructData->getFrames().empty())
	//	return;

	m_curFrame = frame;

	for (int typeNo = 0; typeNo < SRegion::TypeNum; typeNo++)
	{
		QGraphicsItem *item;
		foreach (item, m_pItemGroup[static_cast<SRegion::Type>(typeNo)]->childItems())
			item->hide();
	}

	std::map<SRegion::Type, int> itemCount;
	m_numViewRegion = 0;

	CObjectColor objectColor;
	StructData::iterator<SRegion> it;
	int r, g, b;
	for (it = m_pStructData->regionBegin(m_curCamera, frame); it != m_pStructData->regionEnd(); ++it)
	{
		if (std::find(m_pTargetType->begin(), m_pTargetType->end(), it->type) == m_pTargetType->end())
			continue;
		RectItem *rectItem = 0;
		itemCount[it->type]++;

		if (m_pItemGroup[it->type]->childItems().size() < itemCount[it->type])
		{
			rectItem = new RectItem(m_pItemGroup[it->type]);
			connect(rectItem, SIGNAL(sgItemDoubleClicked(int)), m_pParentDialog, SLOT(onItemDoubleClicked(int)));
		}
		else
			rectItem = qgraphicsitem_cast<RectItem*>(m_pItemGroup[it->type]->childItems()[itemCount[it->type] - 1]);

		if (rectItem)
		{
			rectItem->setSelected(m_pSelectedItems->find(it.getID()) != m_pSelectedItems->end());
			rectItem->setPos(it->rect.x, it->rect.y);
			rectItem->setSize(it->rect.width, it->rect.height);
			rectItem->setTrackIndices(it.getID(), it->frame, it->object, it->type);
			bool bFlag = m_pStructData->checkRegionFlags(it, SRegion::FlagTracked);
			rectItem->setFrameColor(objectColor.getQColor(bFlag, it->type));
			rectItem->show();
			m_numViewRegion++;
		}
	}	// for (it = m_pStructData->regionBegin(0, frame); it != m_pStructData->regionEnd(); ++it)

	//m_mutex.unlock();
}

// ÉJÉÅÉâêÿÇËë÷Ç¶ é∏îsÇµÇΩÇÁfalse
bool MercatorView::changeCamera(int cameraNumber)
{
	bool bret = true;

	m_curCamera = cameraNumber;

	return bret;
}
