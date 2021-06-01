#include "ColorDistView.h"

ColorDistView::ColorDistView(QWidget *parent)
	: IplImageView(parent)
{

}

ColorDistView::~ColorDistView()
{

}

void ColorDistView::addPoint(float x, float y, int r, int g, int b)
{
	points.resize(points.size() + 1);
	cv::Point2f &newPoint = points.back();
	newPoint.x = x;
	newPoint.y = y;

	pointColors.resize(pointColors.size() + 1);
	cv::Point3i &newColor = pointColors.back();
	newColor.x = r;
	newColor.y = g;
	newColor.z = b;
}

void ColorDistView::addEllipse(float x, float y, float rx, float ry, float angle, int r, int g, int b)
{
	ellipse.resize(ellipse.size() + 1);
	PTM::TVector<4, float> &newEllipse = ellipse.back();
	newEllipse[0] = x;
	newEllipse[1] = y;
	newEllipse[2] = rx;
	newEllipse[3] = ry;

	ellipseColors.resize(ellipseColors.size() + 1);
	cv::Point3i &newColor = ellipseColors.back();
	newColor.x = r;
	newColor.y = g;
	newColor.z = b;

	ellipseAngle.push_back(angle);
}

void ColorDistView::drawBackground(QPainter *painter, const QRectF &rect)
{
	IplImageView::drawBackground(painter, rect);

	//QPointF points[2];
	QPen pen;
	pen.setWidth(1);

	for (unsigned int i = 0; i < points.size(); ++i)
	{
		pen.setColor(QColor(pointColors[i].x, pointColors[i].y, pointColors[i].z));
		painter->setPen(pen);
		painter->drawPoint(QPointF(points[i].x, points[i].y));
	}

	for (unsigned int i = 0; i < ellipse.size(); ++i)
	{
		pen.setColor(QColor(ellipseColors[i].x, ellipseColors[i].y, ellipseColors[i].z));
		painter->setPen(pen);
		painter->save();
		painter->translate(ellipse[i][0], ellipse[i][1]);
		painter->rotate(ellipseAngle[i]);
		painter->translate(-ellipse[i][0], -ellipse[i][1]);
		painter->drawEllipse(QPointF(ellipse[i][0], ellipse[i][1]), ellipse[i][2], ellipse[i][3]);
		painter->restore();
	}
	painter->rotate(0);
}