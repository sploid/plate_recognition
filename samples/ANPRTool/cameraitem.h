#ifndef CAMERAITEM_H
#define CAMERAITEM_H

#include <QQuickPaintedItem>

class CameraItem : public QQuickPaintedItem
{
	Q_OBJECT
public:
	CameraItem( QQuickItem* parent = 0 );

protected:
	void paint( QPainter* painter );
};

#endif // CAMERAITEM_H

