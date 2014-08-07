#pragma once
#include <streambuf>
#include <iostream>
#ifdef QT
	#ifdef _MSC_VER
		#pragma warning( push )
		#pragma warning( disable : 4127 4512 4244 )
		#endif // _MSC_VER
		#include <QDebug>
		#ifdef _MSC_VER
		#pragma warning( pop )
	#endif // _MSC_VER
#elif defined ANDROID
	#include <android/log.h>
#endif

class std_cout_2_qdebug : public std::basic_streambuf<char>
{
public:
	std_cout_2_qdebug()
	{
		m_old_buf = std::cout.rdbuf( );
		std::cout.rdbuf( this );
	}

	~std_cout_2_qdebug()
	{
		std::cout.rdbuf( m_old_buf );
	}

protected:
	virtual int_type overflow( int_type v )
	{
		if ( v == '\n' )
		{
#ifdef QT
			qDebug() << endl;
#endif
		}
		return v;
	}


	virtual std::streamsize xsputn( const char *p, std::streamsize n )
	{
		(void)p;
#ifdef QT
		qDebug() << QString::fromLocal8Bit( p, static_cast< int >( n ) );
#elif defined ANDROID
		__android_log_print( ANDROID_LOG_INFO, "JniANPR", "%s\n", p );
#endif
		return n;
	}

private:
	std::streambuf *m_old_buf;
};
