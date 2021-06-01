#ifndef COLORDISTVIEW_H
#define COLORDISTVIEW_H

#include "IplImageView.h"
#include "PTM/TMatrixUtility.h"

class ColorDistView : public IplImageView
{
	Q_OBJECT

public:
	ColorDistView(QWidget *parent);
	~ColorDistView();

private:
	std::vector<cv::Point2f> points;
	std::vector<cv::Point3i> pointColors;

	std::vector<PTM::TVector<4, float> > ellipse;
	std::vector<double> ellipseAngle;
	std::vector<cv::Point3i> ellipseColors;

protected:
	void drawBackground(QPainter *painter, const QRectF &rect);

public:
	void clearPoints() {points.clear(); pointColors.clear();}
	void clearEllipse() {ellipse.clear(); ellipseAngle.clear(); ellipseColors.clear();}
	void addPoint(float x, float y, int r = 255, int g = 255, int b = 255);
	void addEllipse(float x, float y, float rx, float ry, float angle, int r = 255, int g = 255, int b = 255);
};

#endif // COLORDISTVIEW_H
