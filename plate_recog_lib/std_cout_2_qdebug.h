#pragma once
#include "plate_recog_lib_config.h"
#include <streambuf>
#include <iostream>
#include <QDebug>

class PLATE_RECOG_EXPORT std_cout_2_qdebug : public std::basic_streambuf<char>
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
		qDebug() << QString::fromLocal8Bit( p, n );
		return n;
	}

private:
	std::streambuf *m_old_buf;
};
