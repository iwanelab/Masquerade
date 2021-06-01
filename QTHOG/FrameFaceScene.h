#ifndef FRAMEFACESCENE_H
#define FRAMEFACESCENE_H

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>

class FrameFaceScene : public QGraphicsScene
{
	Q_OBJECT

public:
	FrameFaceScene(QObject *parent);
	~FrameFaceScene();

protected:
	//void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent);
	
};

#endif // FRAMEFACESCENE_H
