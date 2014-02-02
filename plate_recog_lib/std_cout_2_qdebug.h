#pragma once
#include <streambuf>
#include <iostream>
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4127 4512 4244 )
#endif
#include <QDebug>
#ifdef _MSC_VER
#pragma warning( pop )
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
			qDebug() << endl;
//			log_window->append("");
		}
		return v;
	}


	virtual std::streamsize xsputn( const char *p, std::streamsize n )
	{
		qDebug() << QString::fromLocal8Bit( p, static_cast< int >( n ) );
		return n;
	}

private:
	std::streambuf *m_old_buf;
};
