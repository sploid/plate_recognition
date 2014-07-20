#include "cameraitem.h"
#include <QPainter>
#include <QTimer>
#include <QTime>
#include <opencv2/opencv.hpp>
#include "../../plate_recog_lib/utils.h"

CameraItem::CameraItem( QQuickItem* parent )
	: QQuickPaintedItem( parent )
	, m_capture( nullptr )
	, m_timer( new QTimer( this ) )
{
	setFlag( QQuickItem::ItemHasContents );
	try
	{
		m_capture = cvCreateCameraCapture( -1 );
		m_timer->setSingleShot( true );
		connect( m_timer, SIGNAL( timeout() ), SLOT( read_frame() ) );
		m_timer->start( 40 );
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
	const QRectF content_rect( contentsBoundingRect() );
	if ( !m_capture || m_last_frame.isNull() )
	{
		painter->fillRect( content_rect, Qt::red );
	}
	else
	{
		QTime begin;
		begin.start();
		const QImage scaled( m_last_frame.scaled( content_rect.size().toSize(), Qt::KeepAspectRatio ) );
		qDebug() << "Scaled: " << begin.elapsed();
		painter->drawImage( 0, 0, scaled );
	}
}

void CameraItem::read_frame()
{
	IplImage* cv_img = cvQueryFrame( m_capture );
	Q_ASSERT( cv_img );
	QTime begin;
	begin.start();
	m_last_frame = mat2qimage( cv_img );
	qDebug() << "Receive next frame, convert: " << begin.elapsed();
	update();
	begin.restart();
	IplImage *destination = cvCreateImage( cvSize((int)((cv_img->width*35)/100) , (int)((cv_img->height*35)/100) ), cv_img->depth, cv_img->nChannels );
   //use cvResize to resize source to a destination image
   cvResize(cv_img, destination);
   qDebug() << "Resize: " << begin.elapsed();

	m_timer->start( 40 );
}


