#pragma once
#include "figure.h"

struct number_data
{
	number_data()
		: m_cache_origin( 0 )
	{
	}
	const cv::Mat* m_cache_origin;
	std::map< std::pair< cv::Rect, bool >, std::pair< char, double >  > m_cache_rets;

private:
	number_data( const number_data& other );
	number_data& operator=( const number_data& other );
};