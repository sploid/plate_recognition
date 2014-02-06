#include "engine.h"
#include <opencv2/opencv.hpp>
#include <assert.h>
#include <stdexcept>
#include "syms.h"
#include "sym_recog.h"
#include "utils.h"
#include "figure.h"

using namespace cv;
using namespace std;


// найденный номер
struct found_number
{
	found_number()
		: m_number( string() )
		, m_weight( -1 )
	{
	}
	string m_number;		// номер
	int m_weight;			// вес
	vector< figure > m_figs;

	pair< string, int > to_pair() const
	{
		return make_pair( m_number, m_weight );
	}

	bool operator < ( const found_number& other ) const
	{
		const int cnp = count_not_parsed_syms();
		const int cnp_other = other.count_not_parsed_syms();
		if ( cnp != cnp_other )
		{
			return cnp > cnp_other;
		}
		else
		{
			return m_weight < other.m_weight;
		}
	}

	bool is_valid() const
	{
		return m_weight != -1 && !m_number.empty();
	}

	int count_not_parsed_syms() const
	{
		if ( m_number.empty() )
			return 100; // вообще ничего нет
		return count( m_number.begin(), m_number.end(), '?' );
	}
};

struct number_data
{
	number_data()
		: m_cache_origin( 0 )
	{
	}
	~number_data()
	{
	}
	const cv::Mat* m_cache_origin;
	std::map< std::pair< cv::Rect, bool >, std::pair< char, double > > m_cache_rets;

private:
	number_data( const number_data& other );
	number_data& operator=( const number_data& other );
};

#define COUNT_GLOBAL_DATA 8
static int g_points_dublicate_first[ COUNT_GLOBAL_DATA ][ 1024 * 1024 * 10 ];
static int g_points_dublicate_second[ COUNT_GLOBAL_DATA ][ 1024 * 1024 * 10 ];
static int g_points_count[ COUNT_GLOBAL_DATA ] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static pair_int g_pix_around[ COUNT_GLOBAL_DATA ][ 4 ] = {
	{ make_pair( 0, 1 ), make_pair( - 1, 0 ), make_pair( 1, 0 ), make_pair( 0, - 1 ) },
	{ make_pair( 0, 1 ), make_pair( - 1, 0 ), make_pair( 1, 0 ), make_pair( 0, - 1 ) },
	{ make_pair( 0, 1 ), make_pair( - 1, 0 ), make_pair( 1, 0 ), make_pair( 0, - 1 ) },
	{ make_pair( 0, 1 ), make_pair( - 1, 0 ), make_pair( 1, 0 ), make_pair( 0, - 1 ) },
	{ make_pair( 0, 1 ), make_pair( - 1, 0 ), make_pair( 1, 0 ), make_pair( 0, - 1 ) },
	{ make_pair( 0, 1 ), make_pair( - 1, 0 ), make_pair( 1, 0 ), make_pair( 0, - 1 ) },
	{ make_pair( 0, 1 ), make_pair( - 1, 0 ), make_pair( 1, 0 ), make_pair( 0, - 1 ) },
	{ make_pair( 0, 1 ), make_pair( - 1, 0 ), make_pair( 1, 0 ), make_pair( 0, - 1 ) } };

#ifdef _WIN32

#include <windows.h>

static bool g_busy_indexes[ COUNT_GLOBAL_DATA ] = { false, false, false, false, false, false, false, false };
HANDLE h_data_guard = CreateMutex( NULL, FALSE, NULL );
int get_free_index()
{
	if ( WaitForSingleObject( h_data_guard, INFINITE ) != WAIT_OBJECT_0 )
	{
		throw runtime_error( "failed call winapi func" );
	}
	for ( int nn = 0; nn < COUNT_GLOBAL_DATA; ++nn )
	{
		if ( !g_busy_indexes[ nn ] )
		{
			g_busy_indexes[ nn ] = true;
			ReleaseMutex( h_data_guard );
			return nn;
		}
	}
	ReleaseMutex( h_data_guard );
	throw runtime_error( "no free data" );
}

void set_free_index( int index )
{
	assert( index >= 0 && index < COUNT_GLOBAL_DATA );
	if ( WaitForSingleObject( h_data_guard, INFINITE ) != WAIT_OBJECT_0 )
	{
		throw runtime_error( "failed call winapi func" );
	}
	g_busy_indexes[ index ] = false;
	ReleaseMutex( h_data_guard );
}

#else

// не поддерживаем многопоточность
int get_free_index()
{
	return 0;
}
void set_free_index( int index )
{
	(void)index;
}

#endif // _WIN32

pair< string, int > read_number_loop( const Mat& input, const vector< int >& search_levels );

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
	const double angl_grad = static_cast< double >( std::abs( angle < 0 ? angle + 10 : angle ) ) * 3.14 / 180.;
	const double koef = static_cast< double >( first_fig_height ) * cos( angl_grad );
	const double move_by_y = koef * 0.2;
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
	etalons.push_back( make_pair( static_cast< int >( 1. * move_by_x ), -1 * static_cast< int >( move_by_y ) ) );
	etalons.push_back( make_pair( static_cast< int >( 1. * move_by_x ), -1 * static_cast< int >( move_by_y ) ) );
	etalons.push_back( make_pair( static_cast< int >( 1. * move_by_x ), -1 * static_cast< int >( move_by_y ) ) );
	etalons.push_back( make_pair( static_cast< int >( 1. * move_by_x ), 0 ) );
	etalons.push_back( make_pair( static_cast< int >( 1. * move_by_x ), 0 ) );

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

pair< char, double > find_sym_nn( bool num, const figure& fig, const Mat& original, number_data& stat_data )
{
	const int left = fig.left();
	assert( left >= 0 );
	int right = left + fig.width() + 1;
	if ( right >= original.cols )
	{
		right = original.cols - 1;
	}

	const int top = fig.top();
	assert( top >= 0 );
	int bottom = top + fig.height() + 1;
	if ( bottom >= original.rows )
	{
		bottom = original.rows - 1;
	}

	const cv::Rect sym_border( left, top, right - left, bottom - top );

	if ( stat_data.m_cache_origin != &original )
	{
		stat_data.m_cache_rets.clear();
		stat_data.m_cache_origin = &original;
	}

	map< pair< cv::Rect, bool >, pair< char, double >  >::const_iterator it = stat_data.m_cache_rets.find( make_pair( sym_border, num ) );
	if ( it != stat_data.m_cache_rets.end() )
	{
		return it->second;
	}

	Mat mm( original.clone(), sym_border );
	pair< char, double >  ret = num ? proc_num( mm ) : proc_char( mm );
	stat_data.m_cache_rets[ make_pair( sym_border, num ) ] = ret;
	return ret;
}

// выбираем пиксели что бы получить контур, ограниченный белыми пикселями
template< int stat_data_index >
void add_pixel_as_spy( int row, int col, Mat& mat, figure& fig, int top_border = -1, int bottom_border = -1 )
{
//	time_mesure tm( "NEXT:" );
	if ( top_border == -1 ) // верняя граница поиска
	{
		top_border = 0;
	}
	if ( bottom_border == -1 ) // нижняя граница поиска
	{
		bottom_border = mat.rows;
	}

	g_points_count[ stat_data_index ] = 0;
	g_points_dublicate_first[ stat_data_index ][ g_points_count[ stat_data_index ] ] = row;
	g_points_dublicate_second[ stat_data_index ][ g_points_count[ stat_data_index ]++ ] = col;

	// что бы не зациклилось
	mat.ptr< unsigned char >( row )[ col ] = 255;
	int cur_index = 0;
	while ( cur_index < g_points_count[ stat_data_index ] )
	{
		const int& cur_nn = g_points_dublicate_first[ stat_data_index ][ cur_index ];
		const int& cur_mm = g_points_dublicate_second[ stat_data_index ][ cur_index ];
		for ( size_t yy = 0; yy < sizeof( g_pix_around[ 0 ] ) / sizeof( g_pix_around[ 0 ][ 0 ] ); ++yy )
		{
			const int &curr_pix_first = g_pix_around[ stat_data_index ][ yy ].first + cur_nn;
			const int &curr_pix_second = g_pix_around[ stat_data_index ][ yy ].second + cur_mm;
			if ( curr_pix_first >= top_border && curr_pix_first < bottom_border
				&& curr_pix_second >= 0 && curr_pix_second < mat.cols )
			{
				if ( mat.ptr< unsigned char >( curr_pix_first )[ curr_pix_second ] == 0 )
				{
					g_points_dublicate_first[ stat_data_index ][ g_points_count[ stat_data_index ] ] = curr_pix_first;
					g_points_dublicate_second[ stat_data_index ][ g_points_count[ stat_data_index ]++ ] = curr_pix_second;
					fig.add_point( pair_int( curr_pix_first, curr_pix_second ) );
					// что бы не зациклилось
					mat.ptr< unsigned char >( curr_pix_first )[ curr_pix_second ] = 255;
				}
			}
		}
		++cur_index;
	}
	assert( fig.is_empty() || fig.left() >= 0 );
}

bool fig_less_left( const figure& lf, const figure& rf )
{
	return lf.left() < rf.left();
}

bool less_by_left_pos( const figure& lf, const figure& rf )
{
	return fig_less_left( lf, rf );
}

void draw_figures( const figure_group& figs, const Mat& etal, const string& key = "figs" )
{
	Mat colored_rect( etal.size(), CV_8UC3 );
	cvtColor( etal, colored_rect, CV_GRAY2RGB );
	for ( size_t mm = 0; mm < figs.size(); ++mm )
	{
		const figure* cur_fig = &figs.at( mm );
		rectangle( colored_rect, Point( cur_fig->left(), cur_fig->top() ), Point( cur_fig->right(), cur_fig->bottom() ), CV_RGB( 0, 255, 0 ) );
	}
	imwrite( next_name( key ), colored_rect );
}

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

found_number find_best_number_by_weight( const vector< found_number >& vals, const Mat* etal = 0 )
{
	(void)etal;
	assert( !vals.empty() );
	if ( vals.empty() )
		return found_number();
	int best_index = 0;
	for ( size_t nn = 1; nn < vals.size(); ++nn )
	{
		if ( vals[ best_index ] < vals[ nn ] )
		{
			best_index = nn;
		}
/*		static int tt = 0;
		cout << tt << " " << vals[ best_index ].number << " " << vals[ best_index ].weight << endl;
		if ( etal )
		{
			Mat colored_rect( etal->size(), CV_8UC3 );
			cvtColor( *etal, colored_rect, CV_GRAY2RGB );
			for ( size_t nn = 0; nn < vals[ best_index ].figs.size(); ++nn )
			{
				const figure* cur_fig = &vals[ best_index ].figs.at( nn );
				rectangle( colored_rect, Point( cur_fig->left(), cur_fig->top() ), Point( cur_fig->right(), cur_fig->bottom() ), CV_RGB( 0, 255, 0 ) );
			}
			imwrite( next_name( "fig" ), colored_rect );
		}
		tt++;*/
	}
	return vals[ best_index ];
}

found_number create_number_by_pos( const vector< pair_int >& pis, const vector< found_symbol >& figs_by_pos )
{
	assert( pis.size() == 6 );
	found_number ret;
	for ( size_t oo = 0; oo < pis.size(); ++oo )
	{
		bool found = false;
		for ( size_t kk = 0; kk < figs_by_pos.size(); ++kk )
		{
			if ( oo == static_cast< size_t >( figs_by_pos[ kk ].pos_in_pis_index ) )
			{
				const found_symbol & cur_sym = figs_by_pos[ kk ];
				ret.m_number += cur_sym.symbol;
				ret.m_weight += static_cast< int >( cur_sym.weight );
				ret.m_figs.push_back( cur_sym.fig );
				found = true;
				break;
			}
		}
		if ( found == false )
		{
			ret.m_number += "?";
		}
	}
	return ret;
}

vector< found_symbol > figs_search_syms( const vector< pair_int >& pis, const pair_int& pos_center, const figure_group& cur_gr, const Mat& original, number_data& stat_data )
{
//	draw_figures( cur_gr, etal );
	vector< found_symbol > ret;
	set< figure > procs_figs;
	assert( pis.size() == 6 );
	pair_int prev_pos = pos_center;
	for ( size_t kk = 0; kk < pis.size(); ++kk )
	{
		const pair_int next_pos = prev_pos + pis.at( kk );
//		draw_point( next_pos, etal );
		found_symbol next;
		next.fig = find_figure( cur_gr, next_pos );
		if ( !next.fig.is_empty()
			&& procs_figs.find( next.fig  ) == procs_figs.end() )
		{
			procs_figs.insert( next.fig );
			const pair< char, double > cc = find_sym_nn( kk >= 1 && kk <= 3, next.fig, original, stat_data );
			assert( cc.first != 0 );
			next.pos_in_pis_index = kk;
			next.symbol = cc.first;
			next.weight = cc.second;
			ret.push_back( next );
			prev_pos = next.fig.center();
		}
		else
		{
			prev_pos = next_pos;
		}
	}
	return ret;
}

vector< found_number > search_number( Mat& etal, vector< figure_group >& groups, const Mat& original, number_data& stat_data )
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
				for ( int oo = -60; oo < 50; oo += 10 )
				{
					const vector< pair_int > pis = calc_syms_centers( ll, oo, cur_fig.height() );
					assert( pis.size() == 6 ); // всегда 6 символов без региона в номере
					const vector< found_symbol > figs_by_pos = figs_search_syms( pis, cen, cur_gr, original, stat_data );
					const found_number number_sum = create_number_by_pos( pis, figs_by_pos );
					fig_nums_sums.push_back( number_sum );
				}
			}
		}

		// выбираем лучшее
		if ( !fig_nums_sums.empty() )
		{
			ret.push_back( find_best_number_by_weight( fig_nums_sums, &etal ) );
		}
	}

	// отрисовываем выбранные группы
/*	for ( size_t nn = 0; nn < groups.size(); ++nn )
	{
		Mat colored_rect( etal.size(), CV_8UC3 );
		cvtColor( etal, colored_rect, CV_GRAY2RGB );
		for ( int mm = 0; mm < groups.at( nn ).size(); ++mm )
		{
			const figure* cur_fig = &groups.at( nn ).at( mm );
			rectangle( colored_rect, Point( cur_fig->left(), cur_fig->top() ), Point( cur_fig->right(), cur_fig->bottom() ), CV_RGB( 0, 255, 0 ) );
		}
		imwrite( next_name( "d" ), colored_rect );
		recog_debug->out_image( colored_rect );
	}*/
	return ret;
}

int fine_best_index( const vector< found_number >& found_nums )
{
	if ( found_nums.empty() )
		return -1;
	else if ( found_nums.size() == 1U )
		return 0;
	int ret = 0;
	for ( size_t nn = 1; nn < found_nums.size(); ++nn )
	{
		if ( found_nums.at( ret ) < found_nums.at( nn ) )
		{
			ret = nn;
		}
	}
	return ret;
}

Mat create_gray_image( const Mat& input )
{
	Mat gray( input.size(), CV_8U );
	if ( input.channels() == 1 )
	{
		gray = input;
	}
	else if ( input.channels() == 3 || input.channels() == 4 )
	{
		cvtColor( input, gray, CV_RGB2GRAY );
	}
	else
	{
		cout << "!!! Invalid chanel count: " << input.channels() << " !!!" << endl;
		assert( !"не поддерживаемое количество каналов" );
	}
	return gray;
}

// бьем картинку на фигуры
template< int stat_data_index >
vector< figure > parse_to_figures( Mat& mat )
{
//	time_mesure to_fig( "TO_FIGS: " );
	vector< figure > ret;
	ret.reserve( 1000 );
	for ( int nn = 0; nn < mat.rows; ++nn )
	{
		for ( int mm = 0; mm < mat.cols; ++mm )
		{
			if ( mat.at< unsigned char >( nn, mm ) == 0 )
			{
				figure fig_to_create;
				add_pixel_as_spy< stat_data_index >( nn, mm, mat, fig_to_create );
				if ( fig_to_create.is_too_big() )
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
/*	if ( !ret.empty() )
	{
		Mat colored_mat( mat.size(), CV_8UC3 );
		cvtColor( mat, colored_mat, CV_GRAY2RGB );
		for ( size_t nn = 0; nn < ret.size(); ++nn )
		{
			rectangle( colored_mat, Point( ret[ nn ].left(), ret[ nn ].top() ), Point( ret[ nn ].right(), ret[ nn ].bottom() ), CV_RGB( 0, 255, 0 ) );
		}
		recog_debug->out_image( colored_mat );
	}*/
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

struct sym_info
{
	sym_info()
		: m_symbol( '?' )
		, m_weight( 0. )
	{
	}
	bool is_valid() const
	{
		return m_symbol != '?' && m_fig.is_empty();
	}
	bool operator==( const figure & other ) const
	{
		return m_fig == other;
	}
	char m_symbol;
	double m_weight;
	figure m_fig;
};

template< int stat_data_index >
void search_region_symbol( found_number& number, const Mat& etal, const Mat& origin, const pair_int& reg_center, const double avarage_height, bool last_symbol, number_data& stat_data )
{
//	number.m_figs.push_back( figure( reg_center, make_pair( 1, 1 ) ) );
	const pair_int nearest_black = search_nearest_black( etal, reg_center );
//	number.m_figs.push_back( figure( nearest_black, make_pair( 1, 1 ) ) );
//	return;

	if ( nearest_black.first != -1
		&& nearest_black.second != -1 )
	{
		figure top_border_fig;
		figure conture_fig;
		Mat to_search = etal.clone();
		// Ищем контуры по верхней границе
		add_pixel_as_spy< stat_data_index >( nearest_black.second, nearest_black.first, to_search, top_border_fig, -1, nearest_black.second + 1 );
		if ( top_border_fig.top() > reg_center.second - static_cast< int >( avarage_height ) ) // ушли не далее чем на одну фигуру
		{
			Mat to_contur = etal.clone();
			add_pixel_as_spy< stat_data_index >( nearest_black.second, nearest_black.first, to_contur, conture_fig, -1, top_border_fig.top() + static_cast< int >( avarage_height ) + 1 );
			if ( conture_fig.width() >= static_cast< int >( avarage_height ) ) // если широкая фигура, то скорее всего захватили рамку
			{
				// центрируем фигуру по Х (вырезаем не большой кусок и определяем его центр)
				Mat to_stable = etal.clone();
				figure short_fig;
				// если центр сильно уехал, этот фокус не сработает, т.к. мы опять захватим рамку
				add_pixel_as_spy< stat_data_index >( nearest_black.second, nearest_black.first, to_stable, short_fig, nearest_black.second - static_cast< int >( avarage_height * 0.4 ), nearest_black.second + static_cast< int >( avarage_height * 0.4 ) );
				conture_fig = figure( pair_int( short_fig.center().first, reg_center.second ), pair_int( static_cast< int >( avarage_height * 0.60 ), static_cast< int >( avarage_height ) ) );
			}
			else if ( last_symbol && conture_fig.width() >= static_cast< int >( avarage_height * 0.80 ) ) // если последняя фигура, то могли захватить болтик черный
			{
				// Тут делаю обработку захвата болтика
				conture_fig = figure( conture_fig.left(), conture_fig.left() + static_cast< int >( avarage_height * 0.60 ), conture_fig.top(), conture_fig.bottom() );
			}
		}
		else
		{
			// центрируем фигуру по Х (вырезаем не большой кусок и определяем его центр)
			Mat to_stable = etal.clone();
			figure short_fig;
			// если центр сильно уехал, этот фокус не сработает, т.к. мы опять захватим рамку
			add_pixel_as_spy< stat_data_index >( nearest_black.second, nearest_black.first, to_stable, short_fig, nearest_black.second - static_cast< int >( avarage_height * 0.4 ), nearest_black.second + static_cast< int >( avarage_height * 0.4 ) );
			if ( !short_fig.is_empty() )
			{
				conture_fig = figure( pair_int( short_fig.center().first, reg_center.second ), pair_int( static_cast< int >( avarage_height * 0.60 ), static_cast< int >( avarage_height ) ) );
			}
			else
			{
				conture_fig = figure();
			}
		}
		if ( !conture_fig.is_empty()
			&& find( number.m_figs.begin(), number.m_figs.end(), conture_fig ) == number.m_figs.end()
			&& static_cast< double >( conture_fig.height() ) > 0.5 * avarage_height
			&& static_cast< double >( conture_fig.width() ) > 0.2 * avarage_height
			)
		{
			const pair< char, double > sym_sym = find_sym_nn( true, conture_fig, origin, stat_data );
			number.m_figs.push_back( conture_fig );
			number.m_number += sym_sym.first;
			number.m_weight += static_cast< int >( sym_sym.second );
		}
		else
		{
			number.m_number += '?';
		}
	}
	else
	{
		number.m_number += '?';
	}
}

pair_int calc_center( const vector< figure >& figs, const vector< vector< pair_doub > >& data, int index )
{
	const static size_t figs_size = 6;
	assert( figs.size() >= figs_size );
	pair_int ret( 0, 0 );
	for ( size_t nn = 0; nn < figs_size; ++nn )
	{
		ret = ret + figs.at( nn ).center() + pair_int( static_cast< int >( data[ nn ][ index ].first ), static_cast< int >( data[ nn ][ index ].second ) );
	}
	ret = ret / figs_size;
	return ret;
}

void apply_angle( const vector< figure >& figs, vector< vector< pair_doub > >& data, double avarage_height, double sin_avarage_angle_by_y )
{
	// перемножаем все коэффициенты что бы получить реальное значение смещения в пикселях
	const double diff_by_x_2_sym = data[ 0 ][ 0 ].first - data[ data.size() - 1 ][ 0 ].first;
	const double koef_by_x_2_sym = ( ( figs.at( figs.size() - 1 ).center().first - figs.at( 0 ).center().first ) / avarage_height ) / diff_by_x_2_sym;
	for ( size_t nn = 0; nn < data.size(); ++nn )
	{
		for ( size_t mm = 0; mm < data[ nn ].size(); ++mm )
		{
			data[ nn ][ mm ].first = data[ nn ][ mm ].first * avarage_height * koef_by_x_2_sym;
			const double angle_offset = sin_avarage_angle_by_y * data[ nn ][ mm ].first;
			data[ nn ][ mm ].second = data[ nn ][ mm ].second * avarage_height + angle_offset;
		}
	}
}

vector< vector< pair_doub > > get_2_sym_reg_koef( const vector< figure >& figs, double avarage_height, double sin_avarage_angle_by_y )
{
	vector< vector< pair_doub > > ret;
	fill_reg( ret, 6.7656, -0.4219, 7.5363, -0.3998 );
	fill_reg( ret, 5.6061, -0.2560, 6.3767, -0.2338 );
	fill_reg( ret, 4.6016, -0.2991, 5.3721, -0.2769 );
	fill_reg( ret, 3.5733, -0.3102, 4.3440, -0.2880 );
	fill_reg( ret, 2.4332, -0.4802, 3.2039, -0.4580 );
	fill_reg( ret, 1.4064, -0.5123, 2.1770, -0.4901 );
	apply_angle( figs, ret, avarage_height, sin_avarage_angle_by_y );
	return ret;
}

vector< vector< pair_doub > > get_3_sym_reg_koef( const vector< figure >& figs, double avarage_height, double sin_avarage_angle_by_y )
{
	vector< vector< pair_doub > > ret;
	fill_reg( ret, 5.8903, -0.3631, 6.6165, -0.3631, 7.3426, -0.3631 );
	fill_reg( ret, 4.9220, -0.2017, 5.6482, -0.2017, 6.3744, -0.2017 );
	fill_reg( ret, 3.9537, -0.2421, 4.6799, -0.2421, 5.4061, -0.2421 );
	fill_reg( ret, 2.9855, -0.2421, 3.7117, -0.2421, 4.4379, -0.2421 );
	fill_reg( ret, 2.0172, -0.4034, 2.7434, -0.4034, 3.4696, -0.4034 );
	fill_reg( ret, 1.0490, -0.4034, 1.7752, -0.4034, 2.5013, -0.4034 );
	apply_angle( figs, ret, avarage_height, sin_avarage_angle_by_y );
	return ret;
}

// выбираем что лучше подходит 2-х или 3-х символьный регион
vector< sym_info > select_sym_info( const vector< sym_info >& fs2, const vector< sym_info >& fs3 )
{
	assert( fs2.size() == 2U );
	assert( fs3.size() == 3U );
	if ( fs3.at( 0 ).m_symbol != '1' && fs3.at( 0 ).m_symbol != '7' )
	{
		return fs2;
	}
	else if ( fs3.at( 0 ).m_symbol == '?' || fs3.at( 1 ).m_symbol == '?' || fs3.at( 2 ).m_symbol == '?' )
	{
		return fs2;
	}
	else
	{
		return fs3;
	}
}

bool not_in_char_distance( int val )
{
	return val <= 0 || val >= 255;
}

bool compare_regions( const found_number& lh, const found_number& rh )
{
	const bool lh_in_reg_list = region_codes().find( lh.m_number ) != region_codes().end();
	const bool rh_in_reg_list = region_codes().find( rh.m_number ) != region_codes().end();
	if ( lh_in_reg_list != rh_in_reg_list )
	{
		return rh_in_reg_list;
	}
	else
	{
		const int cnp = lh.count_not_parsed_syms();
		const int cnp_other = rh.count_not_parsed_syms();
		if ( cnp != cnp_other )
		{
			return cnp > cnp_other;
		}
		else
		{
			return lh.m_weight < rh.m_weight;
		}
	}
}

pair_int get_pos_next_in_region( const vector< figure >& figs, const vector< vector< pair_doub > >& move_reg, const int index, const found_number& number, const double avarage_height )
{
	pair_int ret = calc_center( figs, move_reg, index );
	if ( number.m_number.at( number.m_number.size() - 1 ) != '?' )
	{
		return number.m_figs.at( number.m_figs.size() - 1 ).center() + make_pair( static_cast< int >( 0.75 * avarage_height ), 0 );
	}
	return ret;
}

template< int stat_data_index >
void search_region( found_number& best_number, const int best_level, const Mat& original, number_data& stat_data )
{
	const vector< figure >& figs = best_number.m_figs;
	if ( figs.size() != 6 ) // ищем регион только если у нас есть все 6 символов
	{
		return;
	}

	const double koef_height = 0.76; // отношение высоты буквы к высоте цифры
	// средняя высота буквы
	const double avarage_height = ( static_cast< double >( figs.at( 0 ).height() + figs.at( 4 ).height() + figs.at( 5 ).height() ) / 3.
		+ static_cast< double >( figs.at( 1 ).height() + figs.at( 2 ).height() + figs.at( 3 ).height() ) * koef_height / 3. ) / 2.;

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

	vector< int > levels_to_iterate;
	for ( int nn = best_level - 20; nn <= best_level + 20; nn += 10 )
	{
		levels_to_iterate.push_back( nn );
	}
	levels_to_iterate.erase( remove_if( levels_to_iterate.begin(), levels_to_iterate.end(), &not_in_char_distance ), levels_to_iterate.end() );
//	levels_to_iterate.push_back( best_level + 10 );

	vector< found_number > nums;
	for ( size_t nn = 0; nn < levels_to_iterate.size(); ++nn )
	{
		Mat etal = original > levels_to_iterate.at( nn );
		{
			found_number fs2;
			const vector< vector< pair_doub > > move_reg_by_2_sym_reg( get_2_sym_reg_koef( figs, avarage_height, sin_avarage_angle_by_y ) );
			search_region_symbol< stat_data_index >( fs2, etal, original, calc_center( figs, move_reg_by_2_sym_reg, 0 ), avarage_height, false, stat_data );
			const pair_int next_2 = get_pos_next_in_region( figs, move_reg_by_2_sym_reg, 1, fs2, avarage_height );
			search_region_symbol< stat_data_index >( fs2, etal, original, next_2, avarage_height, true, stat_data );
			nums.push_back( fs2 );
		}
		{
			found_number fs3;
			const vector< vector< pair_doub > > move_reg_by_3_sym_reg( get_3_sym_reg_koef( figs, avarage_height, sin_avarage_angle_by_y ) );
			search_region_symbol< stat_data_index >( fs3, etal, original, calc_center( figs, move_reg_by_3_sym_reg, 0 ), avarage_height, false, stat_data );
			const pair_int next_2 = get_pos_next_in_region( figs, move_reg_by_3_sym_reg, 1, fs3, avarage_height );
/*			pair_int next_2 = calc_center( figs, move_reg_by_3_sym_reg, 1 );
			if ( fs3.m_number.at( fs3.m_number.size() - 1 ) != '?' )
			{
				next_2 = fs3.m_figs.at( fs3.m_figs.size() - 1 ).center() + make_pair( static_cast< int >( 0.75 * avarage_height ), 0 );
			}*/
			search_region_symbol< stat_data_index >( fs3, etal, original, next_2, avarage_height, false, stat_data );
			const pair_int next_3 = get_pos_next_in_region( figs, move_reg_by_3_sym_reg, 2, fs3, avarage_height );
/*			pair_int next_3 = calc_center( figs, move_reg_by_3_sym_reg, 2 );
			if ( fs3.m_number.at( fs3.m_number.size() - 1 ) != '?' )
			{
				next_3 = fs3.m_figs.at( fs3.m_figs.size() - 1 ).center() + make_pair( static_cast< int >( 0.75 * avarage_height ), 0 );
			}*/
			search_region_symbol< stat_data_index >( fs3, etal, original, next_3, avarage_height, true, stat_data );
			nums.push_back( fs3 );
		}
	}
	sort( nums.begin(), nums.end(), &compare_regions );
	vector< found_number >::const_iterator it = max_element( nums.begin(), nums.end(), &compare_regions );

	best_number.m_number.append( it->m_number );
	best_number.m_weight += it->m_weight;
	for ( size_t nn = 0; nn < it->m_figs.size(); ++nn )
	{
		best_number.m_figs.push_back( it->m_figs.at( nn ) );
	}

}

pair< string, int > read_number( const Mat& image, int gray_step )
{
	if ( gray_step <= 0 || gray_step >= 256 )
		gray_step = 10;

	vector< int > search_levels;
	for ( int nn = gray_step; nn < 255; nn += gray_step )
	{
		search_levels.push_back( nn );
	}

	return read_number_loop( image, search_levels );
}

pair< string, int > read_number_by_level( const Mat& image, int gray_level )
{
	vector< int > search_levels;
	if ( gray_level <= 0 || gray_level >= 256 )
		gray_level = 127;

	search_levels.push_back( gray_level );
	return read_number_loop( image, search_levels );
}

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

template< int stat_data_index >
vector< found_number > read_number_impl( const Mat& input, int gray_level, number_data& stat_data )
{
	const Mat& gray_image = create_gray_image( input );
	Mat img_bw = gray_image > gray_level;
	Mat img_to_rez = img_bw.clone();
	//	remove_single_pixels( img_bw );
	// ищем фигуры
	vector< figure > figs = parse_to_figures< stat_data_index >( img_bw );
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
	const vector< found_number > nums = search_number( img_to_rez, groups, gray_image, stat_data );
	return nums;
}

pair< string, int > read_number_loop( const Mat& input, const vector< int >& search_levels )
{
	pair< string, int > ret( make_pair( string(), 0 ) );
	const int free_index = get_free_index();
	try
	{
		number_data stat_data;
		vector< found_number > found_nums;
		for ( size_t nn = 0; nn < search_levels.size(); ++nn )
		{
			vector< found_number > cur_nums;
			switch ( free_index )
			{
			case 0:
				cur_nums = read_number_impl< 0 >( input, search_levels.at( nn ), stat_data );
				break;
			case 1:
				cur_nums = read_number_impl< 1 >( input, search_levels.at( nn ), stat_data );
				break;
			case 2:
				cur_nums = read_number_impl< 2 >( input, search_levels.at( nn ), stat_data );
				break;
			case 3:
				cur_nums = read_number_impl< 3 >( input, search_levels.at( nn ), stat_data );
				break;
			case 4:
				cur_nums = read_number_impl< 4 >( input, search_levels.at( nn ), stat_data );
				break;
			case 5:
				cur_nums = read_number_impl< 5 >( input, search_levels.at( nn ), stat_data );
				break;
			case 6:
				cur_nums = read_number_impl< 6 >( input, search_levels.at( nn ), stat_data );
				break;
			case 7:
				cur_nums = read_number_impl< 7 >( input, search_levels.at( nn ), stat_data );
				break;
			default:
				throw runtime_error( "invalid data index" );
			};

			if ( cur_nums.empty() )
			{
				found_nums.push_back( found_number() );
			}
			else
			{
				found_nums.push_back( cur_nums.at( fine_best_index( cur_nums ) ) );
			}
		}

		const int best_index = fine_best_index( found_nums );
		if ( best_index != -1 )
		{
			found_number& best_number = found_nums.at( best_index );
			const Mat& gray_image = create_gray_image( input );
			const int& best_level = search_levels.at( best_index ) - 10;
			switch ( free_index )
			{
			case 0:
				search_region< 0 >( best_number, best_level, gray_image, stat_data );
				break;
			case 1:
				search_region< 1 >( best_number, best_level, gray_image, stat_data );
				break;
			case 2:
				search_region< 2 >( best_number, best_level, gray_image, stat_data );
				break;
			case 3:
				search_region< 3 >( best_number, best_level, gray_image, stat_data );
				break;
			case 4:
				search_region< 4 >( best_number, best_level, gray_image, stat_data );
				break;
			case 5:
				search_region< 5 >( best_number, best_level, gray_image, stat_data );
				break;
			case 6:
				search_region< 6 >( best_number, best_level, gray_image, stat_data );
				break;
			case 7:
				search_region< 7 >( best_number, best_level, gray_image, stat_data );
				break;
			default:
				throw runtime_error( "invalid data index" );
			};

			// рисуем квадрат номера
/*			Mat num_rect_image = input.clone();
			for ( size_t nn = 0; nn < best_number.m_figs.size(); ++nn )
			{
				const figure& cur_fig = best_number.m_figs[ nn ];
				rectangle( num_rect_image, Point( cur_fig.left(), cur_fig.top() ), Point( cur_fig.right(), cur_fig.bottom() ), CV_RGB( 0, 255, 0 ) );
			}*/

//			imwrite( next_name( "etal" ), num_rect_image );
//			imwrite( next_name( "binary" ), gray_image > search_levels.at( best_index ) );

			ret = best_number.to_pair();
		}
	}
	catch ( const cv::Exception& )
	{
	}
	set_free_index( free_index );
	return ret;
}
