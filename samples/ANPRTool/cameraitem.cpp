#include "cameraitem.h"
#include <QPainter>

CameraItem::CameraItem( QQuickItem* parent )
	: QQuickPaintedItem( parent )
{
	setFlag( QQuickItem::ItemHasContents );
}

void CameraItem::paint( QPainter* painter )
{
	painter->fillRect( contentsBoundingRect(), Qt::red );
}


