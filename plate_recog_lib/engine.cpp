#include "engine.h"
#include <opencv2/opencv.hpp>
#include <assert.h>
#include "syms.h"

using namespace cv;
using namespace std;

typedef pair< int, int > pair_int;
typedef pair< double, double > pair_doub;

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
		return make_pair( hor, ver );
	}
	pair_int top_left() const
	{
		return make_pair( left(), top() );
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
		return make_pair( right(), bottom() );
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

// найденный номер
struct found_number
{
	found_number()
		: number( string() )
		, weight( -1 )
	{
	}
	string number;		// номер
	int weight;			// вес
	vector< figure > figs;
	pair< string, int > to_pair() const
	{
		return make_pair( number, weight );
	}
};

vector< found_number > read_number_impl( const Mat& image, int grey_level, recog_debug_callback *recog_debug );

inline pair_int operator - ( const pair_int& lh, const pair_int& rh )
{
	return make_pair( lh.first - rh.first, lh.second - rh.second );
}

inline pair_int operator + ( const pair_int& lh, const pair_int& rh )
{
	return make_pair( lh.first + rh.first, lh.second + rh.second );
}

inline pair_int operator * ( const pair_int& lh, const double& koef )
{
	return make_pair( static_cast< int >( static_cast< double >( lh.first ) * koef ), static_cast< int >( static_cast< double >( lh.second ) * koef ) );
}

inline pair_int operator / ( const pair_int& lh, const int& koef )
{
	return make_pair( lh.first / koef, lh.second / koef );
}

inline pair_doub operator * ( const pair_doub& lh, const double& koef )
{
	return make_pair( static_cast< double >( lh.first * koef ), static_cast< double >( lh.second * koef ) );
}

// Рассчитываем центры символов
vector< pair_int > calc_syms_centers( int index, int angle, int first_fig_height )
{
	const double angl_grad = static_cast< double >( std::abs( angle ) ) * 3.14 / 180.;
	const double koef = static_cast< double >( first_fig_height ) * cos( angl_grad );
	const double move_by_y = koef * 0.15;
	double move_by_x = 0.;
	vector< pair_int > etalons;
	if ( angle >= 0 )
	{
		move_by_x = koef * 0.96;
	}
	else
	{
		move_by_x = koef * 1.06;
	}
	etalons.push_back( make_pair( static_cast< int >( 1. * move_by_x ), static_cast< int >( move_by_y ) ) );
	etalons.push_back( make_pair( static_cast< int >( 2. * move_by_x ), 0 ) );
	etalons.push_back( make_pair( static_cast< int >( 3. * move_by_x ), 0 ) );
	etalons.push_back( make_pair( static_cast< int >( 4. * move_by_x ), -1 * static_cast< int >( move_by_y ) ) );
	etalons.push_back( make_pair( static_cast< int >( 5. * move_by_x ), 0 ) );

	vector< pair_int > ret;
	const pair_int ret_front( make_pair( 0, 0 ) );
	ret.push_back( ret_front );

	for ( int nn = index - 1; nn >= 0; --nn )
	{
		ret.insert( ret.begin(), ret_front - etalons.at( nn ) );
	}
	for ( int nn = index; nn < 6 - 1; ++nn )
	{
		ret.push_back( ret_front + etalons.at( nn ) );
	}
	return ret;
}

// надо сделать какую-то пропорцию для зависимости угла от расстояния
bool angle_is_equal( int an1, int an2 )
{
	const int angle_diff = 16;
	int an_max = an1 + angle_diff;
	int an_min = an1 - angle_diff;
	if ( an_min >= 0 && an_max <= 360 )
	{
		const bool ret = an2 < an_max && an2 > an_min;
		return ret;
	}
	else if ( an_min <= 0 )
	{
		an_min = an_min + 360;
		return an2 < an_max || an2 > an_min;
	}
	else if ( an_max > 360 )
	{
		an_max = an_max - 360;
		return an2 < an_max || an2 > an_min;
	}
	assert( false );
	return false;
}

typedef map< pair< bool, figure >, pair< char, double > > clacs_figs_type;
clacs_figs_type calcs_figs;

double calc_sym( const Mat& cur_mat, const vector< vector< float > >& sym )
{
	const int wid = sym.at( 0 ).size();
	const int hei = sym.size();
	Mat dest_mat( Size( wid, hei ), CV_8U );
	resize( cur_mat, dest_mat, Size( wid, hei ) );
	dest_mat = dest_mat > 128;
	double sum = 0.;
	for ( int xx = 0; xx < wid; ++xx )
	{
		for ( int yy = 0; yy < hei; ++yy )
		{
			double koef = sym.at( yy ).at( xx );
/*			if ( koef < 0 )
				koef = -1.;
			if ( koef > 0 )
				koef = 1.;*/
//			комбинированный
			//qreal val = 0.;
			//if ( koef < 0 )
			//{
			//	koef = -1.;
			//	// должно быть белым
			//	val = dest_mat.at< quint8 >( yy, xx );
			//}
			//else
			//{
			//	// должно быть черным
			//	val = 255 - dest_mat. at< quint8 >( yy, xx );
			//}
			//sum += koef * val;
//			по белому
//			qreal val = dest_mat. at< quint8 >( yy, xx );
//			sum += -1. * val * koef;
//			по черному
			double val = 255 - dest_mat. at< unsigned char >( yy, xx );
			sum += val * koef;
		}
	}
	return sum;
}

#include "sym_recog.h"

pair< char, double > find_sym( bool num, const figure& fig, const Mat& etal )
{
	if ( num )
	{
		Mat mm( etal, cv::Rect(fig.left(), fig.top(), fig.width(), fig.height()));
		proc( mm );





	}
	clacs_figs_type::const_iterator it = calcs_figs.find( make_pair( num, fig ) );
	if ( it != calcs_figs.end() )
	{
		return it->second;
	}

	typedef vector< vector< float > > sym_def;
	static vector< sym_def > num_syms;
	if ( num_syms.empty() )
	{
		num_syms.push_back( std_sym0() );
		num_syms.push_back( std_sym1() );
		num_syms.push_back( std_sym2() );
		num_syms.push_back( std_sym3() );
		num_syms.push_back( std_sym4() );
		num_syms.push_back( std_sym5() );
		num_syms.push_back( std_sym6() );
		num_syms.push_back( std_sym7() );
		num_syms.push_back( std_sym8() );
		num_syms.push_back( std_sym9() );
	}
	static vector< sym_def > let_syms;
	if ( let_syms.empty() )
	{
		let_syms.push_back( std_symA() );
		let_syms.push_back( std_symB() );
		let_syms.push_back( std_symC() );
		let_syms.push_back( std_symE() );
		let_syms.push_back( std_symH() );
		let_syms.push_back( std_symK() );
		let_syms.push_back( std_symM() );
		let_syms.push_back( std_symO() );
		let_syms.push_back( std_symP() );
		let_syms.push_back( std_symT() );
		let_syms.push_back( std_symX() );
		let_syms.push_back( std_symY() );
	}
	vector< sym_def > * cur_syms = num ? &num_syms : &let_syms;
	// подсчитываем кол-во
	float min_sum_full_syms = 0;
	vector< float > sums_syms_elements;
	for ( size_t nn = 0; nn < cur_syms->size(); ++nn )
	{
		float cur_val = 0;
		for ( size_t mm = 0; mm < cur_syms->at( nn ).size(); ++mm )
		{
			for ( size_t kk = 0; kk < cur_syms->at( nn ).at( mm ).size(); ++kk )
			{
				if ( cur_syms->at( nn ).at( mm ).at( kk ) > 0 )
					cur_val += cur_syms->at( nn ).at( mm ).at( kk );
			}
		}
		sums_syms_elements.push_back( cur_val );
		if ( min_sum_full_syms == 0
			|| min_sum_full_syms > cur_val )
		{
			min_sum_full_syms = cur_val;
		}
	}
	for ( size_t nn = 0; nn < sums_syms_elements.size(); ++nn )
	{
		sums_syms_elements[ nn ] = (float)min_sum_full_syms / sums_syms_elements[ nn ];
	}

	Mat cur_mat( etal, Rect( fig.left(), fig.top(), fig.width() + 1, fig.height() + 1 ) );
	int best_index = -1;
	double max_sum = 0.;
	for ( size_t kk = 0; kk < cur_syms->size(); ++kk )
	{
		const double cur_sum = calc_sym( cur_mat, cur_syms->at( kk ) ) * sums_syms_elements.at( kk );
		if ( cur_sum > max_sum )
		{
			max_sum = cur_sum;
			best_index = kk;
		}
	}
	pair< char, double > ret;

	if ( num )
	{
		switch ( best_index )
		{
		case 0: ret = make_pair( '0', max_sum ); break;
		case 1: ret = make_pair( '1', max_sum ); break;
		case 2: ret = make_pair( '2', max_sum ); break;
		case 3: ret = make_pair( '3', max_sum ); break;
		case 4: ret = make_pair( '4', max_sum ); break;
		case 5: ret = make_pair( '5', max_sum ); break;
		case 6: ret = make_pair( '6', max_sum ); break;
		case 7: ret = make_pair( '7', max_sum ); break;
		case 8: ret = make_pair( '8', max_sum ); break;
		case 9: ret = make_pair( '9', max_sum ); break;
		default: ret = make_pair( (char)0, max_sum ); break;
		}
	}
	else
	{
		switch ( best_index )
		{
		case 0: ret = make_pair( 'A', max_sum ); break;
		case 1: ret = make_pair( 'B', max_sum ); break;
		case 2: ret = make_pair( 'C', max_sum ); break;
		case 3: ret = make_pair( 'E', max_sum ); break;
		case 4: ret = make_pair( 'H', max_sum ); break;
		case 5: ret = make_pair( 'K', max_sum ); break;
		case 6: ret = make_pair( 'M', max_sum ); break;
		case 7: ret = make_pair( 'O', max_sum ); break;
		case 8: ret = make_pair( 'P', max_sum ); break;
		case 9: ret = make_pair( 'T', max_sum ); break;
		case 10: ret = make_pair( 'X', max_sum ); break;
		case 11: ret = make_pair( 'Y', max_sum ); break;
		default: ret = make_pair( (char)0, max_sum ); break;
		}
	}
	calcs_figs[ make_pair( num, fig ) ] = ret;
	return ret;
}

// выбираем пиксели что бы получить контур, ограниченный белыми пикселями
void add_pixel_as_spy( int row, int col, Mat& mat, figure& fig, int top_border = -1, int bottom_border = -1 )
{
	if ( top_border == -1 ) // верняя граница поиска
	{
		top_border = 0;
	}
	if ( bottom_border == -1 ) // нижняя граница поиска
	{
		bottom_border = mat.rows;
	}
	static vector< pair_int > points_dublicate;
	static vector< pair_int > pix_around( 4 );
	static bool init = false;
	if ( !init )
	{
/*		pix_around[ 0 ] = make_pair( cur_nn, cur_mm + 1 );
		pix_around[ 1 ] = make_pair( cur_nn - 1, cur_mm );
		pix_around[ 2 ] = make_pair( cur_nn + 1, cur_mm );
		pix_around[ 3 ] = make_pair( cur_nn, cur_mm - 1 );*/
		pix_around[ 0 ] = make_pair( 0, 1 );
		pix_around[ 1 ] = make_pair( - 1, 0 );
		pix_around[ 2 ] = make_pair( 1, 0 );
		pix_around[ 3 ] = make_pair( 0, - 1 );
		points_dublicate.reserve( 10000 );
		init = true;
	}


	fig.add_point( pair_int( row, col ) );
	points_dublicate.clear();
	points_dublicate.push_back( make_pair( row, col ) );
	// что бы не зациклилось
	mat.at< unsigned char >( row, col ) = 255;
	size_t cur_index = 0;
	while ( cur_index < points_dublicate.size() )
	{
//		pix_around.reserve( 8 );
		const int cur_nn = points_dublicate[ cur_index ].first;
		const int cur_mm = points_dublicate[ cur_index ].second;
		for ( size_t yy = 0; yy < pix_around.size(); ++yy )
		{
			int curr_pix_a[ 2 ] = { pix_around[ yy ].first + cur_nn, pix_around[ yy ].second + cur_mm };
//			const std_pair_int cur_pix( make_pair( pix_around[ yy ].first + cur_nn, pix_around[ yy ].second + cur_mm ) );
			if ( curr_pix_a[ 0 ] >= top_border && curr_pix_a[ 0 ] < bottom_border
				&& curr_pix_a[ 1 ] >= 0 && curr_pix_a[ 1 ] < mat.cols )
			{
				if ( mat.at< unsigned char >( curr_pix_a[ 0 ], curr_pix_a[ 1 ] ) == 0 )
				{
//					if ( !fig.contains( cur_pix ) )
					{
						points_dublicate.push_back( make_pair( curr_pix_a[ 0 ], curr_pix_a[ 1 ] ) );
						fig.add_point( make_pair( curr_pix_a[ 0 ], curr_pix_a[ 1 ] ) );
						// что бы не зациклилось
						mat.at< unsigned char >( curr_pix_a[ 0 ], curr_pix_a[ 1 ] ) = 255;
					}
				}
			}
		}
		++cur_index;
	}
}

bool fig_less_left( const figure& lf, const figure& rf )
{
	return lf.left() < rf.left();
}

bool less_by_left_pos( const figure& lf, const figure& rf )
{
	return fig_less_left( lf, rf );
}

typedef vector< figure > figure_group;

figure find_figure( const figure_group& gr, const pair_int& pos )
{
	for ( size_t nn = 0; nn < gr.size(); ++nn )
	{
		if ( gr[ nn ].top_left() <= pos
			&& gr[ nn ].bottom_right() >= pos )
		{
			return gr[ nn ];
		}
	}
	return figure();
}

template< class T >
set< T > to_set( const vector< T >& in )
{
	set< T > out;
	for ( size_t nn = 0; nn < in.size(); ++nn )
	{
		out.insert( in.at( nn ) );
	}
	return out;
}

// Выкидываем группы, которые включают в себя другие группы
void groups_remove_included( vector< figure_group > & groups )
{
	// todo: переписать постоянное создание set
	bool merge_was = true;
	while ( merge_was )
	{
		merge_was = false;
		for ( size_t nn = 0; nn < groups.size() && !merge_was; ++nn )
		{
			const set< figure > s_cur_f = to_set( groups[ nn ] );
			for ( size_t mm = 0; mm < groups.size() && !merge_was; mm++ )
			{
				if ( nn != mm )
				{
					const set< figure > s_test_f = to_set( groups[ mm ] );
					if ( includes( s_cur_f.begin(), s_cur_f.end(), s_test_f.begin(), s_test_f.end() ) )
					{
						// нашли совпадение
						groups.erase( groups.begin() + mm );
						merge_was = true;
						break;
					}
				}
			}
		}
	}
}

// сливаем пересекающиеся группы
void groups_merge_intersects( vector< figure_group > & groups )
{
	bool merge_was = true;
	while ( merge_was )
	{
		merge_was = false;
		for ( size_t nn = 0; nn < groups.size() && !merge_was; ++nn )
		{
			const set< figure > s_cur_f = to_set( groups[ nn ] );
			for ( size_t mm = 0; mm < groups.size() && !merge_was; ++mm )
			{
				if ( nn != mm )
				{
					set< figure > s_test_f = to_set( groups[ mm ] );
					if ( find_first_of( s_test_f.begin(), s_test_f.end(), s_cur_f.begin(), s_cur_f.end() ) != s_test_f.end() )
					{
						// нашли пересечение
						set< figure > ss_nn = to_set( groups[ nn ] );
						const set< figure > ss_mm = to_set( groups[ mm ] );
						ss_nn.insert( ss_mm.begin(), ss_mm.end() );
						vector< figure > res;
						for ( set< figure >::const_iterator it = ss_nn.begin();
							it != ss_nn.end(); ++it )
						{
							res.push_back( *it );
						}
						groups[ nn ] = res;
						groups.erase( groups.begin() + mm );
						merge_was = true;
						break;
					}
				}
			}
		}
	}
}

void groups_remove_to_small( vector< figure_group > & groups )
{
	for ( int nn = groups.size() - 1; nn >= 0; --nn )
	{
		if ( groups[ nn ].size() < 3 )
		{
			groups.erase( groups.begin() + nn );
		}
	}
}

// выкидываем элементы, что выходят за размеры номера, предпологаем что номер не выше 3 * высота первого элемента
void figs_remote_too_long_by_y_from_first( vector< figure_group > & groups )
{
	for ( size_t nn = 0; nn < groups.size(); ++nn )
	{
		if ( groups[ nn ].size() > 2 )
		{
			const figure& first_fig = groups[ nn ].at( 0 );
			const double height_first = first_fig.height();
			assert( first_fig.height() > 0. );
			for ( int mm = groups[ nn ].size() - 1; mm >= 1; --mm )
			{
				const figure& cur_fig = groups[ nn ][ mm ];
				if ( abs( double( cur_fig.top() - first_fig.top() ) / 3. ) > height_first )
				{
					groups[ nn ].erase( groups[ nn ].begin() + mm );
				}
			}
		}
	}
	// выкидываем группы, где меньше 3-х элементов
	groups_remove_to_small( groups );
}

// выкидываем элементы, что выходят за размеры номера, предпологаем что номер не шире 7 * ширина первого элемента
void figs_remote_too_long_by_x_from_first( vector< figure_group > & groups )
{
	for ( size_t nn = 0; nn < groups.size(); ++nn )
	{
		if ( groups[ nn ].size() > 2 )
		{
			const figure& first_fig = groups[ nn ].at( 0 );
			const double width_first = first_fig.width();
			assert( first_fig.width() > 0. );
			for ( int mm = groups[ nn ].size() - 1; mm >= 1; --mm )
			{
				const figure& cur_fig = groups[ nn ][ mm ];
				if ( double( cur_fig.left() - first_fig.left() ) / 7. > width_first )
				{
					// @todo: тут можно оптимизировать и в начале найти самую допустимо-дальнюю, а уже затем чистить список
					groups[ nn ].erase( groups[ nn ].begin() + mm );
				}
				else
				{
					// тут брик, т.к. фигуры отсортированы по оси х и все предыдущие будут левее
					break;
				}
			}
		}
	}
	// выкидываем группы, где меньше 3-х элементов
	groups_remove_to_small( groups );
}

void figs_remove_invalid_from_first_by_size( vector< figure_group > & groups )
{
	for ( size_t nn = 0; nn < groups.size(); ++nn )
	{
		assert( groups[ nn ].size() > 2 );
		const figure& first_fig = groups[ nn ][ 0 ];
		const double width_first = first_fig.right() - first_fig.left();
		const double height_first = first_fig.bottom() - first_fig.top();
		assert( width_first > 0. );
		for ( int mm = groups[ nn ].size() - 1; mm >= 1; --mm )
		{
			const figure& cur_fig = groups[ nn ][ mm ];
			const double width_cur = cur_fig.right() - cur_fig.left();
			const double height_cur = first_fig.bottom() - first_fig.top();
			if ( width_cur > width_first * 1.5 || width_cur < width_first * 0.6
				|| height_cur > height_first * 1.5 || height_cur < height_first * 0.6 )
			{
				groups[ nn ].erase( groups[ nn ].begin() + mm );
			}
		}
	}
	// выкидываем группы, где меньше 3-х элементов
	groups_remove_to_small( groups );
}

// по отдельным фигурам создаем группы фигур
vector< figure_group > make_groups( vector< figure >& figs )
{
	using namespace std;
	vector< figure_group > groups;
	for ( size_t nn = 0; nn < figs.size(); ++nn )
	{
		vector< pair< double, figure_group > > cur_fig_groups;
		// todo: тут возможно стоит идти только от nn + 1, т.к. фигуры отсортированы и идем только вправо
		for ( size_t mm = 0; mm < figs.size(); ++mm )
		{
			if ( mm != nn )
			{
				if ( figs[ mm ].left() > figs[ nn ].left() )
				{
					// угол между левыми-нижними углами фигуры
					const double angle = 57.2957795 * atan2( static_cast< double >( figs[ mm ].left() - figs[ nn ].left() ), static_cast< double >( figs[ mm ].bottom() - figs[ nn ].bottom() ) );
					bool found = false;
					for ( size_t kk = 0; kk < cur_fig_groups.size(); ++kk )
					{
						// проверяем что попадает в группу
						if ( angle_is_equal( static_cast< int >( cur_fig_groups[ kk ].first ), static_cast< int >( angle ) ) )
						{
							bool ok = true;
							// проверяем что бы угол был такой же как и у всех елементов группы, что бы не было дуги или круга
							for ( size_t yy = 1; yy < cur_fig_groups[ kk ].second.size(); ++yy )
							{
								const figure& next_fig = cur_fig_groups[ kk ].second.at( yy );
								const double angle_to_fig = 57.2957795 * atan2( static_cast< double >( figs[ mm ].left() - next_fig.left() ), static_cast< double >( figs[ mm ].bottom() - next_fig.bottom() ) );
								if ( !angle_is_equal( static_cast< int >( cur_fig_groups[ kk ].first ), static_cast< int >( angle_to_fig ) ) )
								{
									ok = false;
									break;
								}
							}
							if ( ok )
							{
								cur_fig_groups[ kk ].second.push_back( figs[ mm ] );
								found = true;
								break;
							}
						}
					}
					// создаем группу
					if ( !found )
					{
						figure_group to_add;
						to_add.push_back( figs[ nn ] );
						to_add.push_back( figs[ mm ] );
						cur_fig_groups.push_back( make_pair( angle, to_add ) );
					}
				}
			}
		}
		// проверяем что бы элементов в группе было больше 3-х
		for ( size_t mm = 0; mm < cur_fig_groups.size(); ++mm )
		{
			if ( cur_fig_groups[ mm ].second.size() >= 3 )
			{
				// сортируем фигуры в группах, что бы они шли слева направо
				sort( cur_fig_groups[ mm ].second.begin(), cur_fig_groups[ mm ].second.end(), fig_less_left );
				groups.push_back( cur_fig_groups[ mm ].second );
			}
		}
	}
	// выкидываем все группы, у которых угол больше 30 градусов
	for ( int nn = groups.size() - 1; nn >= 0; --nn )
	{
		const double xx = abs( static_cast< double >( groups.at( nn ).at( 0 ).left() - groups.at( nn ).at( 1 ).left() ) );
		const double yy = abs( static_cast< double >( groups.at( nn ).at( 0 ).bottom() - groups.at( nn ).at( 1 ).bottom() ) );
		const double angle_to_fig = abs( 57.2957795 * atan2( yy, xx ) );
		if ( angle_to_fig > 30. && angle_to_fig < 330. )
		{
			groups.erase( groups.begin() + nn );
		}
	}
	return groups;
}

struct found_symbol
{
	figure fig;
	size_t pos_in_pis_index;
	char symbol;
	double weight;
};

found_number find_best_number_by_weight( const vector< found_number >& vals )
{
	assert( !vals.empty() );
	if ( vals.empty() )
		return found_number();
	int best_index = 0;
	for ( size_t nn = 1; nn < vals.size(); ++nn )
	{
		if ( vals[ best_index ].weight < vals[ nn ].weight )
		{
			best_index = nn;
		}
	}
	return vals[ best_index ];
}

found_number create_number_by_pos( const vector< pair_int >& pis, const vector< found_symbol >& figs_by_pos )
{
	assert( pis.size() == 6 );
	found_number ret;
	double weight = 0.; // суммарный вес номера
	for ( size_t oo = 0; oo < pis.size(); ++oo )
	{
		bool found = false;
		for ( size_t kk = 0; kk < figs_by_pos.size(); ++kk )
		{
			if ( oo == static_cast< size_t >( figs_by_pos[ kk ].pos_in_pis_index ) )
			{
				const found_symbol & cur_sym = figs_by_pos[ kk ];
				ret.number += cur_sym.symbol;
				weight += cur_sym.weight;
				ret.figs.push_back( cur_sym.fig );
				found = true;
				break;
			}
		}
		if ( found == false )
		{
			ret.number += "?";
		}
	}
	ret.weight = static_cast< int >( weight ) / 1000;
	return ret;
}

vector< found_symbol > figs_search_syms( const vector< pair_int >& pis, const figure_group& cur_gr, Mat& etal )
{
	vector< found_symbol > ret;
	set< figure > procs_figs;
	for ( size_t kk = 0; kk < pis.size(); ++kk )
	{
		found_symbol next;
		next.fig = find_figure( cur_gr, pis.at( kk ) );
		if ( !next.fig.is_empty()
			&& procs_figs.find( next.fig ) == procs_figs.end() )
		{
			procs_figs.insert( next.fig );
			const pair< char, double > cc = find_sym( kk >= 1 && kk <= 3, next.fig, etal );
			if ( cc.first != 0 )
			{
				next.pos_in_pis_index = kk;
				next.symbol = cc.first;
				next.weight = cc.second;
				ret.push_back( next );
			}
		}
	}
	return ret;
}

vector< found_number > search_number( Mat& etal, vector< figure_group >& groups )
{
	// ищем позиции фигур и соответсвующие им символы
	vector< found_number > ret;
	for ( size_t nn = 0; nn < groups.size(); ++nn )
	{
		vector< found_number > fig_nums_sums;
		const figure_group& cur_gr = groups[ nn ];
		// перебираем фигуры, подставляя их на разные места (пока перебираем только фигуры 0-1-2)
		for ( size_t mm = 0; mm < min( cur_gr.size(), size_t( 2 ) ); ++mm )
		{
			const figure & cur_fig = cur_gr[ mm ];
			const pair_int cen = cur_fig.center();
			// подставляем текущую фигуру на все позиции (пока ставим только первую и вторую фигуру)
			for ( int ll = 0; ll < 1; ++ll )
			{
				// меняем угол наклона номера относительно нас (0 - смотрим прям на номер, если угол меньше 0, то определяем номер с 2-х значным регионом)
				for ( int oo = -50; oo < 50; oo += 10 )
				{
					vector< pair_int > pis = calc_syms_centers( ll, oo, cur_fig.height() );
					for ( size_t kk = 0; kk < pis.size(); ++kk ) // сдвигаем все относительно центра первой фигуры
					{
						pis[ kk ] = pis[ kk ] + cen;
					}
					const vector< found_symbol > figs_by_pos = figs_search_syms( pis, cur_gr, etal );
					const found_number number_sum = create_number_by_pos( pis, figs_by_pos );
					fig_nums_sums.push_back( number_sum );
				}
			}
		}

		// выбираем лучшее
		if ( !fig_nums_sums.empty() )
		{
			ret.push_back( find_best_number_by_weight( fig_nums_sums ) );
		}
	}

	// отрисовываем выбранные группы
/*	for ( int nn = 0; nn < groups.size(); ++nn )
	{
		Mat colored_rect( etal.size(), CV_8UC3 );
		cvtColor( etal, colored_rect, CV_GRAY2RGB );
		for ( int mm = 0; mm < groups.at( nn ).size(); ++mm )
		{
			const std_figure* cur_fig = groups.at( nn ).at( mm );
			rectangle( colored_rect, Point( cur_fig->left(), cur_fig->top() ), Point( cur_fig->right(), cur_fig->bottom() ), CV_RGB( 0, 255, 0 ) );
		}
		recog_debug->out_image( colored_rect );
	}*/
	return ret;
}

int fine_best_level( map< int, found_number >& found_nums )
{
	int ret = -1;
	int best_val = -1;
	for ( map< int, found_number >::const_iterator it = found_nums.begin(); it != found_nums.end(); ++it )
	{
		const int cur_val = it->second.weight;
		if ( cur_val != -1 )
		{
			if ( best_val == -1
				|| found_nums[ ret ].weight < cur_val )
			{
				ret = it->first;
				best_val = cur_val;
			}
		}
	}
	return ret;
}

int find_next_level( map< int, found_number >& found_nums, int gray_step )
{
	if ( gray_step > 0 )
	{
		for ( map< int, found_number >::const_iterator it = found_nums.begin(); it != found_nums.end(); ++it )
		{
			if ( it->second.weight == -1 )
			{
				return it->first;
			}
		}
		return -1;
	}
	else
	{
		const int best_level = fine_best_level( found_nums );
		assert( best_level != -1 );
		// первое
		if ( best_level == found_nums.begin()->first )
		{
			map< int, found_number >::const_iterator it = found_nums.begin();
			++it;
			if ( it->second.weight == -1 )
			{
				// еще не считали
				return it->first;
			}
			else
			{
				// считали и он меньше первого
				return -1;
			}
		}
		// последнее
		map< int, found_number >::const_iterator it_end = found_nums.end();
		--it_end;
		if ( best_level == it_end->first )
		{
			// предпоследнее
			--it_end;
			if ( it_end->second.weight == -1 )
			{
				// еще не считали
				return it_end->first;
			}
			else
			{
				// считали и он меньше последнего
				return -1;
			}
		}
		// лучшее
		map< int, found_number >::const_iterator it_best = found_nums.find( best_level );
		map< int, found_number >::const_iterator it_prev = --found_nums.find( best_level );
		map< int, found_number >::const_iterator it_next = ++found_nums.find( best_level );
		// проверяем что нашли на двух шагах одно и тоже
		if ( it_best->second.number == it_prev->second.number
			|| it_best->second.number == it_next->second.number )
		{
			return -1;
		}
		// если с двух сторо найдено, то ничего не ищем больше
		if ( it_next->second.weight != -1
			&& it_prev->second.weight != -1 )
		{
			return -1;
		}
		// если с двух сторон пусто, то идем вниз
		if ( it_next->second.weight == -1
			&& it_prev->second.weight == -1 )
		{
			return it_prev->first;
		}
		// идем в ту сторону, где не найдено
		if ( it_prev->second.weight == -1 )
		{
			return it_prev->first;
		}
		if ( it_next->second.weight == -1 )
		{
			return it_next->first;
		}
		assert( !"тут не должны быть" );
		return -1;
	}
}

Mat create_binary_mat_by_level( const Mat& input, int gray_level )
{
	Mat gray( input.size(), CV_8U );
	if ( input.channels() == 1 )
	{
		gray = input;
	}
	else if ( input.channels() == 3 )
	{
		cvtColor( input, gray, CV_RGB2GRAY );
	}
	else
	{
		assert( !"не поддерживаемое количество каналов" );
	}
	return gray > gray_level;
}

// бьем картинку на фигуры
vector< figure > parse_to_figures( Mat& mat, recog_debug_callback *recog_debug )
{
	vector< figure > ret;
	ret.reserve( 1000 );
	for ( int nn = 0; nn < mat.rows; ++nn )
	{
		for ( int mm = 0; mm < mat.cols; ++mm )
		{
			if ( mat.at< unsigned char >( nn, mm ) == 0 )
			{
				figure fig_to_create;
				add_pixel_as_spy( nn, mm, mat, fig_to_create );
				if ( fig_to_create.is_valid() )
				{
					// проверяем что высота больше ширины
					if ( fig_to_create.width() < fig_to_create.height() )
					{
						if ( fig_to_create.width() > 4 )
						{
							if ( fig_to_create.height() / fig_to_create.width() < 4 )
							{
								ret.push_back( fig_to_create );
							}
						}
					}
				}
			}
		}
	}
	// отрисовываем найденные фигуры
	if ( !ret.empty() )
	{
		Mat colored_mat( mat.size(), CV_8UC3 );
		cvtColor( mat, colored_mat, CV_GRAY2RGB );
		for ( size_t nn = 0; nn < ret.size(); ++nn )
		{
			rectangle( colored_mat, Point( ret[ nn ].left(), ret[ nn ].top() ), Point( ret[ nn ].right(), ret[ nn ].bottom() ), CV_RGB( 0, 255, 0 ) );
		}
		recog_debug->out_image( colored_mat );
	}
	sort( ret.begin(), ret.end(), less_by_left_pos );
	return ret;
}

void fill_reg( vector< vector< pair_doub > >& data, double x1, double y1, double x2, double y2, double x3, double y3 )
{
	vector< pair_doub > tt;
	tt.push_back( make_pair( x1, y1 ) );
	tt.push_back( make_pair( x2, y2 ) );
	tt.push_back( make_pair( x3, y3 ) );
	data.push_back( tt );
}

void fill_reg( vector< vector< pair_doub > >& data, double x1, double y1, double x2, double y2 )
{
	vector< pair_doub > tt;
	tt.push_back( make_pair( x1, y1 ) );
	tt.push_back( make_pair( x2, y2 ) );
	data.push_back( tt );
}

// ищем ближайшую черную точку
pair_int search_nearest_black( const Mat& etal, const pair_int& center )
{
	pair_int dist_x( center.first, center.first );
	pair_int dist_y( center.second, center.second );
	for (;;)
	{
		for ( int nn = dist_x.first; nn <= dist_x.second; ++nn )
		{
			for ( int mm = dist_y.first; mm <= dist_y.second; ++mm )
			{
				const unsigned char cc = etal.at< unsigned char >( mm, nn );
				if ( cc != 255 )
				{
					assert( cc == 0 );
					return pair_int( nn, mm );
				}
			}
		}
		--dist_x.first;
		++dist_x.second;
		--dist_y.first;
		++dist_y.second;
		if ( dist_x.first < 0 || dist_x.second >= etal.cols
			|| dist_y.first < 0 || dist_y.second >= etal.rows )
		{
			return pair_int( -1, -1 );
		}
	}
}

void add_region( found_number& best_number, const Mat& etal, const pair_int& reg_center, const double avarage_height )
{
	const pair_int nearest_black = search_nearest_black( etal, reg_center );
	if ( nearest_black.first != -1
		|| nearest_black.second != -1 )
	{
		figure top_border_fig;
		Mat to_search = etal.clone();
		// Ищем контуры по нижней границе
		add_pixel_as_spy( nearest_black.second, nearest_black.first, to_search, top_border_fig, -1, nearest_black.second + 1 );
		if ( top_border_fig.top() > reg_center.second - static_cast< int >( avarage_height ) ) // ушли не далее чем на одну фигуру
		{
			Mat to_contur = etal.clone();
			figure conture_fig;
			add_pixel_as_spy( nearest_black.second, nearest_black.first, to_contur, conture_fig, -1, top_border_fig.top() + static_cast< int >( avarage_height ) + 1 );
			// если у нас рамочка наезжает на цифры, то у нас будет очень широкая фигура
			if ( conture_fig.width() >= static_cast< int >( avarage_height ) )
			{
				// центрируем фигуру по Х (вырезаем не большой кусок и определяем его центр)
				Mat to_stable = etal.clone();
				figure short_fig;
				// если центр сильно уехал, этот фокус не сработает, т.к. мы опять захватим рамку
				add_pixel_as_spy( nearest_black.second, nearest_black.first, to_stable, short_fig, nearest_black.second - static_cast< int >( avarage_height * 0.4 ), nearest_black.second + static_cast< int >( avarage_height * 0.4 ) );
				conture_fig = figure( pair_int( short_fig.center().first, reg_center.second), pair_int( static_cast< int >( avarage_height * 0.65 ), static_cast< int >( avarage_height ) ) );
			}
			best_number.figs.push_back( conture_fig );
			const pair< char, double > sym_sym = find_sym( true, conture_fig, etal );
			if ( sym_sym.first != 0 )
			{
				best_number.number += sym_sym.first;
			}
			else
			{
				best_number.number += '?';
			}
		}
		else
		{
			// не нашли верхней границы
			best_number.number += '?';
		}
	}
	else
	{
		best_number.number += '?';
	}
}

void search_region( found_number& best_number, const Mat& etal )
{
	const vector< figure >& figs = best_number.figs;
	if ( figs.size() != 6 ) // ищем регион только если у нас есть все 6 символов
	{
		return;
	}

	int sum_dist = 0;
	for ( int nn = 5; nn >= 1; --nn )
	{
		sum_dist += figs.at( nn ).center().first - figs.at( nn - 1 ).center().first;
	}
	// средняя дистанция между символами
	const double avarage_distance = static_cast< double >( sum_dist ) / 5.;
	const double koef_height = 0.76; // отношение высоты буквы к высоте цифры
	// средняя высота буквы
	const double avarage_height = ( static_cast< double >( figs.at( 0 ).height() + figs.at( 4 ).height() + figs.at( 5 ).height() ) / 3.
		+ static_cast< double >( figs.at( 1 ).height() + figs.at( 2 ).height() + figs.at( 3 ).height() ) * koef_height / 3. ) / 2.;
	const double div_sym_hei_to_dis = avarage_distance / avarage_height;
	const double koef_move_by_2_sym_reg = 1.06;
	// смещение относительно расстояния между первым и вторым символом
	double koef_by_x = ( ( figs.at( 5 ).center().first - figs.at( 0 ).center().first ) / avarage_height ) / 5.3496;
// 	if ( div_sym_hei_to_dis > koef_move_by_2_sym_reg )
// 	{
// 		// точно 2-х символьный регион и угол наклона 0
// 	}
// 	else
// 	{
// 		const double koef_move_by_3_sym_reg = 0.96;
// 		// угол наклона номера, если у нас 3-х символьный регион
// 		const double angle_3_sym_reg = div_sym_hei_to_dis > koef_move_by_3_sym_reg ? 0. : acos( div_sym_hei_to_dis / koef_move_by_3_sym_reg );
// 		const double cos_angle_3_sym_reg = cos( angle_3_sym_reg );
		// коэффициенты смещения символов региона из 2-х букв относительно остальных символов номера
		vector< vector< pair_doub > > move_reg_by_2_sym_reg;
		fill_reg( move_reg_by_2_sym_reg, 6.7656, -0.4219, 7.5363, -0.3998 );
		fill_reg( move_reg_by_2_sym_reg, 5.6061, -0.2560, 6.3767, -0.2338 );
		fill_reg( move_reg_by_2_sym_reg, 4.6016, -0.2991, 5.3721, -0.2769 );
		fill_reg( move_reg_by_2_sym_reg, 3.5733, -0.3102, 4.3440, -0.2880 );
		fill_reg( move_reg_by_2_sym_reg, 2.4332, -0.4802, 3.2039, -0.4580 );
		fill_reg( move_reg_by_2_sym_reg, 1.4064, -0.5123, 2.1770, -0.4901 );

// 		// коэффициенты смещения символов региона из 3-х букв относительно остальных символов номера
// 		vector< vector< pair_doub > > move_reg_by_3_sym_reg;
// 		fill_reg( move_reg_by_3_sym_reg, 5.8903, -0.3631, 6.6165, -0.3631, 7.3426, -0.3631 );
// 		fill_reg( move_reg_by_3_sym_reg, 4.9220, -0.2017, 5.6482, -0.2017, 6.3744, -0.2017 );
// 		fill_reg( move_reg_by_3_sym_reg, 3.9537, -0.2421, 4.6799, -0.2421, 5.4061, -0.2421 );
// 		fill_reg( move_reg_by_3_sym_reg, 2.9855, -0.2421, 3.7117, -0.2421, 4.4379, -0.2421 );
// 		fill_reg( move_reg_by_3_sym_reg, 2.0172, -0.4034, 2.7434, -0.4034, 3.4696, -0.4034 );
// 		fill_reg( move_reg_by_3_sym_reg, 1.0490, -0.4034, 1.7752, -0.4034, 2.5013, -0.4034 );

		// ищем угол наклона по высоте (считаем среднее между не соседними фигурами 0-4, 0-5, 1-3)
		vector< pair_int > index_fig_to_angle;
		index_fig_to_angle.push_back( make_pair( 0, 4 ) );
		index_fig_to_angle.push_back( make_pair( 0, 5 ) );
		index_fig_to_angle.push_back( make_pair( 1, 3 ) );
		double avarage_angle_by_y = 0.; // средний угол наклона номера по оси У
		int move_by_y = 0; // номер наклонен вверх или вниз
		for ( size_t nn = 0; nn < index_fig_to_angle.size(); ++nn )
		{
			const pair_int& cur_in = index_fig_to_angle.at( nn );
			avarage_angle_by_y += atan( static_cast< double >( figs[ cur_in.first ].center().second - figs[ cur_in.second ].center().second ) / static_cast< double >( figs[ cur_in.first ].center().first - figs[ cur_in.second ].center().first ) );
			move_by_y += figs[ cur_in.first ].center().second - figs[ cur_in.second ].center().second;
		}
		avarage_angle_by_y = avarage_angle_by_y / static_cast< double >( index_fig_to_angle.size() );
		const double sin_avarage_angle_by_y = sin( avarage_angle_by_y );

		// перемножаем все коэффициенты что бы получить реальное значение смещения в пикселях
		for ( size_t nn = 0; nn < move_reg_by_2_sym_reg.size(); ++nn )
		{
			for ( size_t mm = 0; mm < move_reg_by_2_sym_reg[ nn ].size(); ++mm )
			{
				move_reg_by_2_sym_reg[ nn ][ mm ].first = move_reg_by_2_sym_reg[ nn ][ mm ].first * avarage_height * koef_by_x;
				const double angle_offset = sin_avarage_angle_by_y * move_reg_by_2_sym_reg[ nn ][ mm ].first;
				move_reg_by_2_sym_reg[ nn ][ mm ].second = move_reg_by_2_sym_reg[ nn ][ mm ].second * avarage_height + angle_offset;
			}
		}
// 		for ( size_t nn = 0; nn < move_reg_by_3_sym_reg.size(); ++nn )
// 		{
// 			for ( size_t mm = 0; mm < move_reg_by_3_sym_reg[ nn ].size(); ++mm )
// 			{
// 				move_reg_by_3_sym_reg[ nn ][ mm ] = move_reg_by_3_sym_reg[ nn ][ mm ] * avarage_height * cos_angle_3_sym_reg;
// 			}
// 		}

		// рисуем позиции 2-х символьного региона
		const size_t figs_size = figs.size();
		pair_int sum_first( 0, 0 );
		pair_int sum_second( 0, 0 );
		for ( size_t nn = 0; nn < figs_size; ++nn )
		{
			sum_first = sum_first + figs.at( nn ).center() + pair_int( static_cast< int >( move_reg_by_2_sym_reg[ nn ][ 0 ].first ), static_cast< int >( move_reg_by_2_sym_reg[ nn ][ 0 ].second ) );
			sum_second = sum_second + figs.at( nn ).center() + pair_int( static_cast< int >( move_reg_by_2_sym_reg[ nn ][ 1 ].first ), static_cast< int >( move_reg_by_2_sym_reg[ nn ][ 1 ].second ) );
		}
		sum_first = sum_first / 6;
		sum_second = sum_second / 6;

		// ищем цифры 2-х символьного региона
		add_region( best_number, etal, sum_first, avarage_height );
		add_region( best_number, etal, sum_second, avarage_height );
//	}
}

pair< string, int > read_number_loop( const Mat& image, map< int, found_number >& found_nums, recog_debug_callback *recog_debug, int gray_step )
{
	int next_level = find_next_level( found_nums, gray_step );
	while ( next_level != -1 )
	{
		const vector< found_number > cur_nums = read_number_impl( image, next_level, recog_debug );
		if ( cur_nums.empty() )
		{
			found_number num_zero;
			num_zero.weight = 0;
			found_nums[ next_level ] = num_zero;
		}
		else
		{
			found_nums[ next_level ] = find_best_number_by_weight( cur_nums );
		}
		next_level = find_next_level( found_nums, gray_step );
	}

	const int best_level = fine_best_level( found_nums );
	if ( best_level != -1 )
	{
		assert( best_level != -1 );
		found_number& best_number = found_nums[ best_level ];
		search_region( best_number, create_binary_mat_by_level( image, best_level ) );

		// рисуем квадрат номера
		Mat num_rect_image = image.clone();
		for ( size_t nn = 0; nn < best_number.figs.size(); ++nn )
		{
			const figure& cur_fig = best_number.figs[ nn ];
			rectangle( num_rect_image, Point( cur_fig.left(), cur_fig.top() ), Point( cur_fig.right(), cur_fig.bottom() ), CV_RGB( 0, 255, 0 ) );
		}
		recog_debug->out_image( num_rect_image );

		imwrite("C:\\imgs\\debug\\0.png", num_rect_image);

		return best_number.to_pair();
	}
	else
	{
		return make_pair( string(), 0 );
	}
}

pair< string, int > read_number( const Mat& image, recog_debug_callback *recog_debug, int gray_step )
{
	assert( recog_debug );
	vector< int > first_search_levels;
	if ( gray_step <= 0 )
	{
		first_search_levels.push_back( 127 );
		first_search_levels.push_back( 63 );
		first_search_levels.push_back( 191 );
		first_search_levels.push_back( 31 );
		first_search_levels.push_back( 95 );
		first_search_levels.push_back( 159 );
		first_search_levels.push_back( 223 );
		first_search_levels.push_back( 15 );
		first_search_levels.push_back( 47 );
		first_search_levels.push_back( 79 );
		first_search_levels.push_back( 111 );
		first_search_levels.push_back( 143 );
		first_search_levels.push_back( 175 );
		first_search_levels.push_back( 207 );
		first_search_levels.push_back( 239 );
	}
	else
	{
		for ( int nn = gray_step; nn < 255; nn += gray_step )
		{
			first_search_levels.push_back( nn );
		}
	}
//	Проверить почему у мерина последняя цифра региона определилась не верно 

	map< int, found_number > found_nums;
	for ( size_t nn = 0; nn < first_search_levels.size(); ++nn )
	{
		found_nums[ first_search_levels[ nn ] ] = found_number();
	}

	for ( size_t nn = 0; nn < first_search_levels.size(); ++nn )
	{
		const vector< found_number > cur_nums = read_number_impl( image, first_search_levels.at( nn ), recog_debug );
		if ( !cur_nums.empty() )
		{
			const found_number best_num = find_best_number_by_weight( cur_nums );
			found_nums[ first_search_levels.at( nn ) ] = best_num;
			if ( best_num.weight != 0 )
			{
				return read_number_loop( image, found_nums, recog_debug, gray_step );
			}
		}
	}
	return make_pair( string( "" ), 0 );
}

pair< string, int > read_number_by_level( const Mat& image, int gray_level, recog_debug_callback *recog_debug )
{
	const vector< found_number > fn = read_number_impl( image, gray_level, recog_debug );
	return fn.empty() ? make_pair( string(), 0 ) : find_best_number_by_weight( fn ).to_pair();
}

// вырезаем одиночные пиксели
void remove_single_pixels( Mat& mat )
{
	for ( int nn = 0; nn < mat.rows; ++nn )
	{
		for ( int mm = 0; mm < mat.cols - 2; ++mm )
		{
			unsigned char c1 = mat.at< unsigned char >( nn, mm );
			unsigned char c2 = mat.at< unsigned char >( nn, mm + 1 );
			unsigned char c3 = mat.at< unsigned char >( nn, mm + 2 );
			if ( c1 == c3 && c2 != c1 )
			{
				mat.at< unsigned char >( nn, mm + 1 ) = c1;
			}
		}
	}
	for ( int nn = 0; nn < mat.cols; ++nn )
	{
		for ( int mm = 0; mm < mat.rows - 2; ++mm )
		{
			unsigned char c1 = mat.at< unsigned char >( mm, nn );
			unsigned char c2 = mat.at< unsigned char >( mm + 1, nn );
			unsigned char c3 = mat.at< unsigned char >( mm + 2, nn );
			if ( c1 == c3 && c2 != c1 )
			{
				mat.at< unsigned char >( mm + 1, nn ) = c1;
			}
		}
	}
}

vector< found_number > read_number_impl( const Mat& input, int gray_level, recog_debug_callback *recog_debug )
{
	calcs_figs.clear();
	Mat img_bw = create_binary_mat_by_level( input, gray_level );
	Mat img_to_rez = img_bw.clone();
//	remove_single_pixels( img_bw );
	// ищем фигуры
	vector< figure > figs = parse_to_figures( img_bw, recog_debug );
	// бьем по группам в зависимости от угла наклона элементов отностительно друг друга
	vector< figure_group > groups = make_groups( figs );
	// выкидываем элементы, что выходят за размеры номера, предпологаем что номер не шире 7 * ширина первого элемента
	figs_remote_too_long_by_x_from_first( groups );
	// выкидываем элементы, которые слишком далеко по высоте от предыдущего
	figs_remote_too_long_by_y_from_first( groups );
	// выкидываем элементы, не пропорциональные первому элементу
	figs_remove_invalid_from_first_by_size( groups );
	// Выкидываем группы, которые включают в себя другие группы
	groups_remove_included( groups );
	// сливаем пересекающиеся группы
	groups_merge_intersects( groups );
	// ищем номера
	const vector< found_number > nums = search_number( img_to_rez, groups );
	return nums;
}
