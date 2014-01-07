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
	const pair< string, int > num = read_number_by_level( QImage2cvMat( cur_image() ), ui.m_le_row_num->text().toInt() );
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

cv::Mat recog_test::QImage2cvMat( const QImage& q_image )
{
	const int width = q_image.width();
	const int height = q_image.height();
	const int count_channels = q_image.bytesPerLine() == q_image.width() + 1 ? 1 : 3;
	if ( count_channels == 3 )
	{
		cv::Mat ret( cvSize( width, height ), CV_8UC3 );
		for ( int y = 0; y < height; ++y )
		{
			for ( int i = 0; i < width; ++i )
			{
				const QColor next_pixel = q_image.pixel( i, y );
				ret.at< Vec3b >( y, i ) = Vec3b( static_cast< unsigned char >( next_pixel.blue() ), static_cast< unsigned char >( next_pixel.green() ), static_cast< unsigned char >( next_pixel.red() ) );
			}
		}
		return ret;
	}
	else if ( count_channels == 1 )
	{
		Mat ret( cvSize( width, height ), CV_8U );
		for ( int y = 0; y < height; ++y )
		{
			for ( int i = 0; i < width; ++i )
			{
				const int gray_pixel = qGray( q_image.pixel( i, y ) );
				ret.at< unsigned char >( y, i ) = static_cast< unsigned char >( gray_pixel );
			}
		}
		return ret;
	}
	else
	{
		Q_ASSERT(false);
		return cv::Mat();
	}
}

QImage recog_test::Mat2QImage( const cv::Mat& mat )
{
	const int height = mat.rows;
	const int width = mat.cols;
	if ( mat.depth() == CV_MAT_DEPTH( CV_8U ) && mat.channels() == 3 )
	{
		const QImage ret( (const uchar*)mat.ptr(), width, height, QImage::Format_RGB888 );
		return ret.rgbSwapped();
	}
	else if ( mat.depth() == CV_MAT_DEPTH( CV_8U ) && mat.channels() == 1 )
	{
		QImage ret( width, height, QImage::Format_RGB32 );
		for( int nn = 0; nn < height; ++nn )
		{
			for ( int mm = 0; mm < width; ++mm )
			{
				const unsigned char next_pixel = static_cast< int >( mat.at< unsigned char >( nn, mm ) );
				ret.setPixel( mm, nn, qRgb( next_pixel, next_pixel, next_pixel ) );
			}
		}
		return ret;
	}
	else if ( mat.depth() == CV_MAT_DEPTH( CV_32F ) && mat.channels() == 1 )
	{
		Q_ASSERT(!"сомневаюсь что это работает");
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