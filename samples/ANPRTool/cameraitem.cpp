#include "cameraitem.h"
#include <QPainter>
#include <QTimer>
#include <opencv2/opencv.hpp>
#include "../../plate_recog_lib/utils.h"

CameraItem::CameraItem( QQuickItem* parent )
	: QQuickPaintedItem( parent )
	, m_capture( nullptr )
{
	setFlag( QQuickItem::ItemHasContents );
	try
	{
		m_capture = cvCreateCameraCapture( -1 );
		QTimer* timer = new QTimer( this );
		connect( timer, SIGNAL( timeout() ), SLOT( read_frame() ) );
		timer->start( 40 );
	}
	catch (...)
	{
		qWarning() << "Failed init camera";
	}
}

CameraItem::~CameraItem()
{
	if ( m_capture )
	{
		cvReleaseCapture( &m_capture );
	}
}

void CameraItem::paint( QPainter* painter )
{
	if ( !m_capture || m_last_frame.isNull() )
	{
		painter->fillRect( contentsBoundingRect(), Qt::red );
	}
	else
	{
		painter->drawImage( 0, 0, m_last_frame );
	}
}

void CameraItem::read_frame()
{
	IplImage* cv_img = cvQueryFrame( m_capture );
	Q_ASSERT( cv_img );
	m_last_frame = mat2qimage( cv_img );
	qDebug() << "Receive next frame ";
	update();
}


