#ifndef OBJECTFACEVIEW_H
#define OBJECTFACEVIEW_H

#include "FaceView.h"

class ObjectFaceView : public FaceView
{
	Q_OBJECT

public:
	ObjectFaceView(QWidget *parent);
	~ObjectFaceView();

	void updateView() {updateView(true);}
	void updateView(bool bCenterOn);
	void setScale(float scale);

private:
	float m_scale;

protected:
	enum {CacheSize = 20, FaceCacheSize = 1000000};

protected:
	virtual void drawBackground(QPainter *painter, const QRectF &rect);

private:
	std::vector<int> m_gridX;
	std::vector<int> m_gridY;
	std::vector<int> m_gridWidth;
	std::vector<int> m_gridHeight;
	std::set<int> m_selectedObjects;
	std::list<int> m_cacheFrames;
	std::list<int> m_cacheFaces;
	std::map<int, cv::Mat> m_cacheImages;
	std::map<int, cv::Mat> m_faceCacheImages;
	std::map<int, CvRect> m_faceCacheRects;
	int m_firstFrame;
	int m_lastFrame;

private:
	void calcGrid();
};

#endif // OBJECTFACEVIEW_H
