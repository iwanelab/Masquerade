#ifndef FRAMEFACEVIEW_H
#define FRAMEFACEVIEW_H

#include <QResizeEvent>
#include "FaceView.h"

class FrameFaceView : public FaceView//QGraphicsView
{
	Q_OBJECT

protected:
	virtual void resizeEvent(QResizeEvent *event);

public:
	FrameFaceView(QWidget *parent = 0);
	~FrameFaceView();

public:
	void updateView();
	void setScale(float scale) {m_scale = scale; alignFaces();}
	void alignFaces(int w = -1, int h = -1);
};

#endif // FRAMEFACEVIEW_H
