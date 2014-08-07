#pragma once

#include <opencv2/opencv.hpp>

typedef std::pair< int, int > pair_int;
typedef std::pair< double, double > pair_doub;

inline std::string next_name( const std::string& key, const std::string& ext = "png" )
{
	using namespace std;
	static map< string, int > cache;
	map< string, int >::iterator it = cache.find( key );
	if ( it == cache.end() )
	{
		cache[ key ] = 0;
		it = cache.find( key );
	}
	std::stringstream ss;
#ifdef ANDROID
	ss << "/storage/sdcard0/imgs/" << key << "_" << it->second++ << "." << ext;
#else
	ss << key << it->second++ << "." << ext;
#endif
	return ss.str();
}

inline void draw_point( const pair_int& pi, const cv::Mat& etal, const std::string& key = "pi" )
{
	using namespace cv;
	Mat colored_rect( etal.size(), CV_8UC3 );
	cvtColor( etal, colored_rect, CV_GRAY2RGB );
	rectangle( colored_rect, Point( pi.first - 1, pi.second - 1 ), Point( pi.first + 1, pi.second + 1 ), CV_RGB( 0, 255, 0 ) );
	imwrite( next_name( key ), colored_rect );
}

inline void draw_points( const std::vector< pair_int >& pis, const cv::Mat& etal, const std::string& key = "pts" )
{
	using namespace cv;
	Mat colored_rect( etal.size(), CV_8UC3 );
	cvtColor( etal, colored_rect, CV_GRAY2RGB );
	for ( size_t mm = 0; mm < pis.size(); ++mm )
	{
		rectangle( colored_rect, Point( pis.at( mm ).first - 1, pis.at( mm ).second - 1 ), Point( pis.at( mm ).first + 1, pis.at( mm ).second + 1 ), CV_RGB( 0, 255, 0 ) );
	}
	imwrite( next_name( key ), colored_rect );
}

namespace cv
{
	inline bool operator < ( const cv::Rect& lh, const cv::Rect& rh )
	{
		return memcmp( &lh, &rh, sizeof( cv::Rect ) ) < 0;
	}
}

template< class T >
std::set< T > to_set( const std::vector< T >& in )
{
	std::set< T > out;
	for ( size_t nn = 0; nn < in.size(); ++nn )
	{
		out.insert( in.at( nn ) );
	}
	return out;
}

class time_mesure
{
public:
	time_mesure( const std::string& title )
		: m_title( title )
		, m_begin( cv::getTickCount() )
	{
	}
	~time_mesure()
	{
		std::cout << m_title << " " << ( static_cast< double >(cv::getTickCount() - m_begin)/cv::getTickFrequency() ) << std::endl;
	}
private:
	time_mesure( const time_mesure& other );
	time_mesure& operator=( const time_mesure& other );
	const std::string m_title;
	const int64 m_begin;
};

#ifdef QT_GUI
inline QImage mat2qimage( const cv::Mat& mat, const QRect& rect = QRect() )
{
	using namespace cv;
	const int height = mat.rows;
	const int width = mat.cols;
	Mat dest;
	if ( rect.isNull() )
	{
		dest = mat;
	}
	else
	{
		cv::resize( mat, dest, cv::Size( 10, 10 ) );
	}
	if ( dest.depth() == CV_MAT_DEPTH( CV_8U ) && dest.channels() == 3 )
	{
		const QImage ret( static_cast<const uchar*>( dest.ptr() ), width, height, QImage::Format_RGB888 );
		return ret.rgbSwapped();
	}
	else if ( dest.depth() == CV_MAT_DEPTH( CV_8U ) && dest.channels() == 1 )
	{
		QImage ret( width, height, QImage::Format_RGB32 );
		for( int nn = 0; nn < height; ++nn )
		{
			for ( int mm = 0; mm < width; ++mm )
			{
				const unsigned char next_pixel = static_cast< int >( dest.at< unsigned char >( nn, mm ) );
				ret.setPixel( mm, nn, qRgb( next_pixel, next_pixel, next_pixel ) );
			}
		}
		return ret;
	}
	else if ( dest.depth() == CV_MAT_DEPTH( CV_32F ) && dest.channels() == 1 )
	{
		Q_ASSERT( !"I doubt that it works" );
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
				const int next_pixel = static_cast< int >( dest.at< float >( nn, mm ) );
				ret.setPixel( mm, nn, next_pixel );
			}
		}
		return ret;
	}
	Q_ASSERT( !"not supported format" );
	return QImage();
}
#endif
