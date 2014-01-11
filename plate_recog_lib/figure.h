#pragma once
#include <vector>
#include "utils.h"

class figure
{
public:
	figure()
		: m_left( -1 )
		, m_right( -1 )
		, m_top( -1 )
		, m_bottom( -1 )
		, m_too_big( false )
	{
	}
	figure( pair_int center, pair_int size )
		: m_left( center.first - size.first / 2 - 1 )
		, m_right( center.first + size.first / 2 + 1 )
		, m_top( center.second - size.second / 2 - 1 )
		, m_bottom( center.second + size.second / 2 + 1 )
		, m_too_big( false )
	{
	}
	figure( int left, int right, int top, int bottom )
		: m_left( left )
		, m_right( right )
		, m_top( top )
		, m_bottom( bottom )
	{
	}
	int width() const
	{
		return m_right - m_left;
	}
	int height() const
	{
		return m_bottom - m_top;
	}
	bool is_empty() const
	{
		return m_left == -1 && m_right == -1 && m_top == -1 && m_bottom == -1;
	}
	void add_point( const pair_int& val )
	{
		if ( too_big() )
			return;
		if ( m_left == - 1 || m_left > val.second )
		{
			m_left = val.second;
		}
		if ( m_top == - 1 || m_top > val.first )
		{
			m_top = val.first;
		}
		if ( m_right == - 1 || m_right < val.second )
		{
			m_right = val.second;
		}
		if ( m_bottom == - 1 || m_bottom < val.first )
		{
			m_bottom = val.first;
		}
		if ( right() - left() > 50 )
		{
			m_too_big = true;
		}
	}
	pair_int center() const
	{
		const int hor = m_left + ( m_right - m_left ) / 2;
		const int ver = m_top + ( m_bottom - m_top ) / 2;
		return std::make_pair( hor, ver );
	}
	pair_int top_left() const
	{
		return std::make_pair( left(), top() );
	}
	int left() const
	{
		return m_left;
	}
	int top() const
	{
		return m_top;
	}
	int right() const
	{
		return m_right;
	}
	int bottom() const
	{
		return m_bottom;
	}
	pair_int bottom_right() const
	{
		return std::make_pair( right(), bottom() );
	}
	bool is_valid() const
	{
		return !m_too_big && !is_empty();
	}
	bool too_big() const
	{
		return m_too_big;
	}
	bool operator<( const figure & other ) const
	{
		if ( m_left != other.m_left )
			return m_left < other.m_left;
		else if ( m_top != other.m_top )
			return m_top < other.m_top;
		else if ( m_right != other.m_right )
			return m_right < other.m_right;
		else if ( m_bottom != other.m_bottom )
			return m_bottom < other.m_bottom;
		else
			return false;
	}
	bool operator==( const figure & other ) const
	{
		return m_left == other.m_left
			&& m_right == other.m_right
			&& m_top == other.m_top
			&& m_bottom == other.m_bottom;
	}

private:
	int m_left;
	int m_right;
	int m_top;
	int m_bottom;
	bool m_too_big;
};

//typedef std::map< std::pair< bool, figure >, std::pair< char, double > > clacs_figs_type;
typedef std::vector< figure > figure_group;
