#include "recog_test.h"
#include "syms.h"
#include "engine.h"
using namespace cv;
using namespace std;

recog_test::recog_test( QWidget* parent )
	: QDialog( parent )
	, m_cur_image( 0 )
{
	ui.setupUi( this );
	setWindowFlags( windowFlags() &~ Qt::WindowContextHelpButtonHint );

	QSettings set;
	set.beginGroup( "SaveDirs" );
	const QString last_file_name = set.value( "LastOpenFile" ).toString();
	if ( !last_file_name.isEmpty() )
	{
		QImage im( last_file_name );
		if ( !im.isNull() )
		{
			m_images.push_back( im );
		}
		update_image();
	}
}

void recog_test::update_image()
{
	const QImage ci = cur_image();
	if ( !ci.isNull() )
	{
		ui.lb_cur_image->setText( QString( "%1 (%2)" ).arg( m_cur_image + 1 ).arg( m_images.size() ) );
		ui.m_lb_image->setPixmap( QPixmap::fromImage( ci ) );
	}
}

QImage recog_test::cur_image()
{
	bool b1 = m_cur_image >= 0;
	bool b2 = m_cur_image < m_images.size();
	if ( b1 && b2 )
	{
		return m_images.at( m_cur_image );
	}
	const QImage ret = ( b1 && b2 ? m_images.at( m_cur_image ) : QImage() );
	return ret;
}

void recog_test::on_m_but_right_clicked()
{
	if ( m_cur_image < m_images.size() - 1 )
	{
		++m_cur_image;
		update_image();
	}
}

void recog_test::on_m_but_left_clicked()
{
	if ( m_cur_image > 0 )
	{
		--m_cur_image;
		update_image();
	}
}

void recog_test::on_m_but_run_clicked()
{
	using namespace std;
	const pair< string, int > num = read_number_by_level( QImage2IplImage( cur_image() ), ui.m_le_row_num->text().toInt(), this );
	stringstream stream;
	if ( num.first.empty() )
	{
		stream << "Not found";
	}
	else
	{
		stream << "Found number: " << num.first << ", weight: " << num.second;
	}
	QMessageBox::information( this, "Recog", stream.str().c_str() );
}

void recog_test::on_m_but_remove_cur_image_clicked()
{
	if ( m_cur_image >= 0 && m_cur_image < m_images.size() )
	{
		m_images.removeAt( m_cur_image );
		update_image();
	}
}

void recog_test::out_image( const cv::Mat & m )
{
	m_images.insert( m_cur_image + 1, Mat2QImage( m ) );
	++m_cur_image;
	update_image();
}

void recog_test::out_string( const std::string& text )
{
	qDebug() << text.c_str();
}

void recog_test::on_m_but_load_image_clicked()
{
	QSettings set;
	set.beginGroup( "SaveDirs" );
	const QString file_name = QFileDialog::getOpenFileName( this, "Open file", set.value( "OpenImageDir", "" ).toString() );
	if ( !file_name.isEmpty() )
	{
		QImage im( file_name );
		if ( !im.isNull() )
		{
			set.setValue( "OpenImageDir", QFileInfo( file_name ).absoluteFilePath() );
			set.setValue( "LastOpenFile", file_name );
			const QImage cur_im = cur_image();
			m_images.insert( cur_im.isNull() ? m_cur_image : m_cur_image + 1, im );
			if ( !cur_im.isNull() )
				++m_cur_image;
			update_image();
		}
	}
}

IplImage* recog_test::QImage2IplImage( const QImage& qImage )
{
	int width = qImage.width();
	int height = qImage.height();

	// Creates a iplImage with 3 channels
	IplImage *img = cvCreateImage( cvSize( width, height ), IPL_DEPTH_8U, 3 );
	char * imgBuffer = img->imageData;

	//Remove alpha channel
	int jump = qImage.bytesPerLine() / qImage.width();// ( qImage.hasAlphaChannel() ) ? 4 : 3;

	for ( int y = 0; y < img->height; ++y )
	{
		QByteArray a( (const char*)qImage.scanLine( y ), qImage.bytesPerLine() );
		for ( int i = 0; i < a.size(); i += jump )
		{
			//Swap from RGB to BGR
			imgBuffer[ 2 ] = a[ i + 2 ];
			imgBuffer[ 1 ] = a[ i + 1 ];
			imgBuffer[ 0 ] = a[ i ];
			imgBuffer += 3;
		}
	}

	return img;
}

QImage recog_test::Mat2QImage( const cv::Mat& mat )
{
	const int height = mat.rows;
	const int width = mat.cols;

	if ( mat.depth() == CV_MAT_DEPTH( CV_8U ) && mat.channels() == 3 )
	{
		const uchar *qImageBuffer = (const uchar*)mat.ptr();
		QImage img(qImageBuffer, width, height, QImage::Format_RGB888);
		return img.rgbSwapped();
	}
	else if ( mat.depth() == CV_MAT_DEPTH( CV_8U ) && mat.channels() == 1 )
	{
		const uchar *qImageBuffer = (const uchar*)mat.ptr();
		QImage img( qImageBuffer, width, height, QImage::Format_Indexed8 );
		QVector< QRgb > colorTable;
		for ( int i = 0; i < 256; ++i )
		{
			colorTable.push_back( qRgb( i, i, i ) );
		}
		img.setColorTable( colorTable );
		return img;
	}
	else if ( mat.depth() == CV_MAT_DEPTH( CV_32F ) && mat.channels() == 1 )
	{
		QImage ret( width, height, QImage::Format_Indexed8 );
		QVector< QRgb > colorTable;
		for ( int i = 0; i < 256; ++i )
		{
			colorTable.push_back( qRgb( i, i, i ) );
		}
		ret.setColorTable( colorTable );
		for( int nn = 0; nn < height; ++nn )
		{
			for ( int mm = 0; mm < width; ++mm )
			{
				const int next_pixel = static_cast< int >( mat.at< float >( nn, mm ) );
				ret.setPixel(mm, nn, next_pixel);
			}
		}
		return ret;
	}
	Q_ASSERT(!"not supported format");
	return QImage();
}