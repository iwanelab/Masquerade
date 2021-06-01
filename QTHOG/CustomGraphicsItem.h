#ifndef CUSTOMGRAPHICSITEM_H
#define CUSTOMGRAPHICSITEM_H

#include <QGraphicsItem>

class CustomGraphicsItem : public QObject, public QGraphicsItem
{
	Q_OBJECT

public:
	CustomGraphicsItem(QGraphicsItem *parent = 0);
	~CustomGraphicsItem();

public:
	enum ItemTypes {Rect = UserType + 1, Line, Feature, Corner, VPoint, Image};

private:
	
};

#endif // CUSTOMGRAPHICSITEM_H
