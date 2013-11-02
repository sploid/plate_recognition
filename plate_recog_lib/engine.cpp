#include "engine.h"
#include <opencv2/opencv.hpp>
#include <assert.h>

using namespace cv;
using namespace std;

std::vector< std::pair< std::string, int > > read_number( const cv::Mat& image, int grey_level, recog_debug_callback *recog_debug );

typedef std::pair< int, int > std_pair_int;


inline std_pair_int operator-( const std_pair_int& lh, const std_pair_int& rh )
{
	return make_pair( lh.first - rh.first, lh.second - rh.second );
}

inline std_pair_int operator+( const std_pair_int& lh, const std_pair_int& rh )
{
	return make_pair( lh.first + rh.first, lh.second + rh.second );
}

inline std_pair_int operator*( const std_pair_int& lh, const double& koef )
{
	return make_pair( static_cast< int >( static_cast< double >( lh.first ) * koef ), static_cast< int >( static_cast< double >( lh.second ) * koef ) );
}


vector< std_pair_int > std_calc_centers( int index, float move_koef )
{
	vector< std_pair_int > ret;
// 1 - 37x46
// 2 - 33x56
	vector< std_pair_int > etalons;
	etalons.push_back( make_pair( 43, 7 ) );
	etalons.push_back( make_pair( 38, 0 ) );
	etalons.push_back( make_pair( 38, 0 ) );
	etalons.push_back( make_pair( 47, -7 ) );
	etalons.push_back( make_pair( 44, 0 ) );
	ret.push_back( make_pair( 0, 0 ) );
	for ( int nn = index - 1; nn >= 0; --nn )
	{
		ret.insert( ret.begin(), ret.front() - etalons.at( nn ) );
	}
	for ( int nn = index; nn < 6 - 1; ++nn )
	{
		ret.push_back( ret.back() + etalons.at( nn ) );
	}
	for ( size_t nn = 0; nn < ret.size(); ++nn )
	{
		ret[ nn ] = ret.at( nn ) * move_koef;
	}
// 1-2 - 43x7
// 2-3 - 38
// 3-4 - 38
// 4-5 - 47x7
// 5-6 - 44
//	return qMakePair( static_cast< int >( xx * move_koef ), static_cast< int >( yy * move_koef ) );
	return ret;
}

// надо сделать какую-то пропорцию для зависимости угла от расстояния
bool std_angle_is_equal( int an1, int an2 )
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


class std_figure
{
public:
	std_figure()
		: m_left( -1 )
		, m_right( -1 )
		, m_top( -1 )
		, m_bottom( -1 )
		, m_too_big( false )
	{
	}
	int width() const { return m_right - m_left; }
	int height() const { return m_bottom - m_top; }
	void add_point( const std_pair_int& val )
	{
		if ( too_big() )
			return;
		//m_points.insert( val );
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
			//m_points.clear();
		}
	}
	std_pair_int center() const
	{
		const int hor = ( m_right - m_left ) / 2;
		const int ver = ( m_bottom - m_top ) / 2;
		return make_pair( hor, ver );
	}
	std_pair_int top_left() const
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
	std_pair_int bottom_right() const
	{
		return make_pair( right(), bottom() );
	}
	bool is_valid() const
	{
		return !m_too_big
			&& m_left != -1
			&& m_right != -1
			&& m_top != -1
			&& m_bottom != -1;
//			&& m_points.size();
	}
	bool too_big() const
	{
		return m_too_big;
	}
private:
	int m_left;
	int m_right;
	int m_top;
	int m_bottom;
	bool m_too_big;
//	set< std_pair_int > m_points;
};

#include "syms.h"

double std_calc_sym( const cv::Mat& cur_mat, const vector< vector< float > >& sym )
{
	const int wid = sym.at( 0 ).size();
	const int hei = sym.size();
	cv::Mat dest_mat( cv::Size( wid, hei ), CV_8U );
	cv::resize( cur_mat, dest_mat, Size( wid, hei ) );
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


pair< char, double > std_find_sym( bool num, const std_figure* fig, const cv::Mat& etal )
{
	typedef map< pair< bool, const std_figure* >, pair< char, double > > clacs_figs_type;
	static clacs_figs_type calcs_figs;
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

	cv::Mat cur_mat( etal, cv::Rect( fig->left(), fig->top(), fig->width() + 1, fig->height() + 1 ) );
	int best_index = -1;
	double max_sum = 0.;
	for ( size_t kk = 0; kk < cur_syms->size(); ++kk )
	{
		const double cur_sum = std_calc_sym( cur_mat, cur_syms->at( kk ) ) * sums_syms_elements.at( kk );
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


void add_pixel_as_spy( int nn, int mm, cv::Mat& mat, std_figure& fig )
{
	static vector< std_pair_int > points_dublicate;
	static vector< std_pair_int > pix_around( 4 );
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


	fig.add_point( std_pair_int( nn, mm ) );
	points_dublicate.clear();
	points_dublicate.push_back( make_pair( nn, mm ) );
	// что бы не зациклилось
	mat.at< unsigned char >( nn, mm ) = 255;
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
			if ( curr_pix_a[ 0 ] >= 0 && curr_pix_a[ 0 ] < mat.rows
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

bool std_less_left( const std_figure* lf, const std_figure* rf )
{
	return lf->left() < rf->left();
}

bool std_less_left_ref( const std_figure& lf, const std_figure& rf )
{
	return std_less_left( &lf, &rf );
}

typedef vector< const std_figure* > std_group;

const std_figure* std_find_figure( const std_group& gr, const std_pair_int& pos )
{
	for ( size_t nn = 0; nn < gr.size(); ++nn )
	{
		if ( gr.at( nn )->top_left() <= pos
			&& gr.at( nn )->bottom_right() >= pos )
		{
			return gr.at( nn );
		}
	}
	return 0;
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


std::vector< std::pair< string, int > > procces_found_figs( vector< std_figure > figs, cv::Mat& etal, recog_debug_callback *recog_debug )
{
	(void)recog_debug;
	vector< pair< string, int > > ret;
	sort( figs.begin(), figs.end(), std_less_left_ref );
	// бьем по группам
	vector< std_group > groups;
	for ( size_t nn = 0; nn < figs.size(); ++nn )
	{
		vector< pair< double, std_group > > cur_fig_groups;
		// todo: тут возможно стоит идти только от nn + 1, т.к. фигуры отсортированы и идем только вправо
		for ( size_t mm = 0; mm < figs.size(); ++mm )
		{
			if ( mm != nn )
			{
				if ( figs[ mm ].left() > figs[ nn ].left() )
				{
					const double angle = 57.2957795 * atan2( static_cast< double >( figs[ mm ].left() - figs[ nn ].left() ), static_cast< double >( figs[ mm ].bottom() - figs[ nn ].bottom() ) );
					bool found = false;
					for ( size_t kk = 0; kk < cur_fig_groups.size(); ++kk )
					{
						// проверяем что попадает в группу
						if ( std_angle_is_equal( static_cast< int >( cur_fig_groups[ kk ].first ), static_cast< int >( angle ) ) )
						{
							bool ok = true;
							// проверяем что бы угол был такой же как и у всех елементов группы, что бы не было дуги или круга
							for ( size_t yy = 1; yy < cur_fig_groups[ kk ].second.size(); ++yy )
							{
								const std_figure * next_fig = cur_fig_groups[ kk ].second.at( yy );
								const double angle_to_fig = 57.2957795 * atan2( static_cast< double >( figs[ mm ].left() - next_fig->left() ), static_cast< double >( figs[ mm ].bottom() - next_fig->bottom() ) );
								if ( !std_angle_is_equal( static_cast< int >( cur_fig_groups[ kk ].first ), static_cast< int >( angle_to_fig ) ) )
								{
									ok = false;
									break;
								}
							}
							if ( ok )
							{
								cur_fig_groups[ kk ].second.push_back( &figs[ mm ] );
								found = true;
								break;
							}
						}
					}
					// создаем группу
					if ( !found )
					{
						std_group to_add;
						to_add.push_back( &figs[ nn ] );
						to_add.push_back( &figs[ mm ] );
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
				groups.push_back( cur_fig_groups[ mm ].second );
			}
		}
	}
	// сортируем фигуры в группах
	for ( size_t nn = 0; nn < groups.size(); ++nn )
	{
		sort( groups[ nn ].begin(), groups[ nn ].end(), std_less_left );
	}
	// выкидываем элементы, что выходят за размеры номера, предпологаем что номер не шире 7 * ширина первого элемента
	// todo: !!!!!!!!!!!!!если номер будет наклонным, то ширина будет не пропорциональная высоте (надо вводить косинусь угла наклона)!!!!!!!!!!!!!!!!!!!
	for ( size_t nn = 0; nn < groups.size(); ++nn )
	{
		if ( groups[ nn ].size() > 2 )
		{
			const std_figure* first_fig = groups[ nn ].at( 0 );
			const double width_first = first_fig->right() - first_fig->left();
			assert( width_first > 0. );
			for ( int mm = groups[ nn ].size() - 1; mm >= 1; --mm )
			{
				const std_figure* cur_fig = groups[ nn ][ mm ];
				if ( double( cur_fig->left() - first_fig->left() ) / 7. > width_first )
				{
					groups[ nn ].erase( groups[ nn ].begin() + mm );
				}
				else
				{
					break;
				}
			}
		}
	}
	// выкидываем группы, где меньше 3-х элементов
	for ( int nn = groups.size() - 1; nn >= 0; --nn )
	{
		if ( groups[ nn ].size() < 3 )
		{
			groups.erase( groups.begin() + nn );
		}
	}

	// выкидываем элементы, не пропорциональные первому элементу
	for ( size_t nn = 0; nn < groups.size(); ++nn )
	{
		if ( groups[ nn ].size() > 2 )
		{
			const std_figure* first_fig = groups[ nn ][ 0 ];
			const double width_first = first_fig->right() - first_fig->left();
			const double height_first = first_fig->bottom() - first_fig->top();
			assert( width_first > 0. );
			for ( int mm = groups[ nn ].size() - 1; mm >= 1; --mm )
			{
				const std_figure* cur_fig = groups[ nn ][ mm ];
				const double width_cur = cur_fig->right() - cur_fig->left();
				const double height_cur = first_fig->bottom() - first_fig->top();
				if ( width_cur > width_first * 1.5 || width_cur < width_first * 0.6
					|| height_cur > height_first * 1.5 || height_cur < height_first * 0.6 )
				{
					groups[ nn ].erase( groups[ nn ].begin() + mm );
				}
			}
		}
	}
	// выкидываем группы, где меньше 3-х элементов
	for ( int nn = groups.size() - 1; nn >= 0; --nn )
	{
		if ( groups[ nn ].size() < 3 )
		{
			groups.erase( groups.begin() + nn );
		}
	}

	// мерджим группы
	// todo: переписать постоянное создание QSet
	bool merge_was = true;
	while ( merge_was )
	{
		merge_was = false;
		for ( size_t nn = 0; nn < groups.size() && !merge_was; ++nn )
		{
			const set< const std_figure* > s_cur_f = to_set( groups[ nn ] );
			for ( size_t mm = 0; mm < groups.size() && !merge_was; mm++ )
			{
				if ( nn != mm )
				{
					const set< const std_figure* > s_test_f = to_set( groups[ mm ] );
					if ( std::includes( s_cur_f.begin(), s_cur_f.end(), s_test_f.begin(), s_test_f.end() ) )
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
	// todo: сливаем пересекающиеся группы
	merge_was = true;
	while ( merge_was )
	{
		merge_was = false;
		for ( size_t nn = 0; nn < groups.size() && !merge_was; ++nn )
		{
			const set< const std_figure* > s_cur_f = to_set( groups[ nn ] );
			for ( size_t mm = 0; mm < groups.size() && !merge_was; ++mm )
			{
				if ( nn != mm )
				{
					set< const std_figure* > s_test_f = to_set( groups[ mm ] );
					if ( find_first_of( s_test_f.begin(), s_test_f.end(), s_cur_f.begin(), s_cur_f.end() ) != s_test_f.end() )
					{
						// нашли пересечение
						set< const std_figure* > ss_nn = to_set( groups[ nn ] );
						const set< const std_figure* > ss_mm = to_set( groups[ mm ] );
						ss_nn.insert( ss_mm.begin(), ss_mm.end() );
						vector< const std_figure* > res;
						for ( set< const std_figure* >::const_iterator it = ss_nn.begin();
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
	// ищем позиции фигур и соответсвующие им символы
	for ( size_t nn = 0; nn < groups.size(); ++nn )
//	size_t nn = 0U;
	{
		vector< pair< string, int > > fig_nums_sums;
		const std_group& cur_gr = groups[ nn ];
		// перебираем фигуры, подставляя их на разные места
		for ( size_t mm = 0; mm < std::min( cur_gr.size(), size_t( 2 ) ); ++mm )
//		int mm = 0;
		{
			const std_figure * cur_fig = cur_gr[ mm ];
			const std_pair_int cen = cur_fig->center();
			// подставляем текущую фигуру на все позиции
			for ( int ll = 0; ll < 1; ++ll )
//			int ll = 2;
			{
				// todo: 52 НЕВЕРНО, ПОСТАВИТЬ ПРАВИЛЬНОЕ (35-41-46)
				for ( int oo = 30; oo < 50; ++oo )
				{
					const float move_koef = static_cast< float >( cur_fig->height() ) / ( ll >= 1 && ll <= 3 ? (float)( oo + 15 ) : (float)oo );

					vector< std_pair_int > pis = std_calc_centers( ll, move_koef );

//					рисуем точки
//					Mat colored_gr( etal.size(), CV_8UC3 );
//					cvtColor( etal, colored_gr, CV_GRAY2RGB );

					for ( size_t kk = 0; kk < pis.size(); ++kk )
					{
						pis[ kk ] = pis[ kk ] + cen + cur_fig->top_left();
//						cv::circle( colored_gr, Point( pis[ kk ].first, pis[ kk ].second ), 1, CV_RGB( 255, 0, 0 ) );
					}
//					recog_debug->out_image( colored_gr );
					set< const std_figure* > procs_figs;
					vector< pair< pair< const std_figure*, int >, pair< char, double > > > figs_by_pos;
					for ( size_t kk = 0; kk < pis.size(); ++kk )
					{
						const std_figure* ff = std_find_figure( cur_gr, pis.at( kk ) );
						if ( ff
							&& procs_figs.find( ff ) == procs_figs.end() )
						{
							procs_figs.insert( ff );

							const pair< char, double > cc = std_find_sym( kk >= 1 && kk <= 3, ff, etal );

							if ( cc.first != 0 )
							{
								pair< const std_figure*, int > fig_pos = make_pair( ff, kk );
								figs_by_pos.push_back( make_pair( fig_pos, cc ) );
							}
						}
					}
					double figure_sum = 0.;
					string number;
					for ( size_t oo = 0; oo < pis.size(); ++oo )
					{
						bool found = false;
						for ( size_t kk = 0; kk < figs_by_pos.size(); ++kk )
						{
							if ( oo == static_cast< size_t >( figs_by_pos[ kk ].first.second ) )
							{
								number += figs_by_pos[ kk ].second.first;
								figure_sum += figs_by_pos[ kk ].second.second;
								found = true;
								break;
							}
						}
						if ( found == false )
						{
							number += "?";
						}
					}
					fig_nums_sums.push_back( make_pair( number, ( (int)figure_sum / 1000 ) ) );
				}
			}
		}

		// выбираем лучшее
		int best_index = -1;
		float best_sum = 0.;
		(void)best_sum;
		for ( size_t nn = 0; nn < fig_nums_sums.size(); ++nn )
		{
			if ( best_index == - 1 )
			{
				best_index = 0;
			}
			else
			{
				if ( fig_nums_sums[ nn ].second > fig_nums_sums[ best_index ].second )
				{
					best_index = nn;
				}
			}
		}
		if ( best_index != -1 )
		{

/*			пытался посчитать общую сумму, это оставим на потом
 *			int found_syms = 0;
			const std::string test_num = fig_nums_sums.at( best_index ).first;
			for ( int rr = 0; rr < test_num.size(); ++rr )
			{
				if ( test_num.at( rr ) != '?' )
				{
					++found_syms;
				}
			}
			if ( found_syms * 23 <= fig_nums_sums.at( best_index ).second )
			{*/
				ret.push_back( fig_nums_sums.at( best_index ) );
				stringstream out;
				out << "next figure: " << fig_nums_sums.at( best_index ).first << " " << fig_nums_sums.at( best_index ).second;
				recog_debug->out_string( out.str() );
/*			}
			else
			{
				qDebug() << "not found number in group";
			}*/
		}
		else
		{
			recog_debug->out_string( "not found number in group" );
		}
	}

	// todo: пытаемся определить что где стоит

	// отрисовываем выбранные группы
/*	for ( int nn = 0; nn < groups.size(); ++nn )
	{
		Mat colored_rect( etal.size(), CV_8UC3 );
		cvtColor( etal, colored_rect, CV_GRAY2RGB );
		for ( int mm = 0; mm < groups.at( nn ).size(); ++mm )
		{
			const std_figure* cur_fig = groups.at( nn ).at( mm );
			cv::rectangle( colored_rect, Point( cur_fig->left(), cur_fig->top() ), Point( cur_fig->right(), cur_fig->bottom() ), CV_RGB( 0, 255, 0 ) );
		}
		recog_debug->out_image( colored_rect );
	}*/
	return ret;
}

std::pair< std::string, int > find_best( const std::vector< std::pair< std::string, int > >& vals )
{
	int best_index = -1;
	for ( size_t nn = 0; nn < vals.size(); ++nn )
	{
		if ( best_index == -1
			|| vals[ best_index ].second < vals[ nn ].second )
		{
			best_index = nn;
		}
	}
	return best_index == -1 ? std::make_pair( std::string(), 0 ) : vals[ best_index ];
}

int fine_best_level( map< int, pair< string, int > >& found_nums )
{
	int ret = -1;
	int best_val = -1;
	for ( map< int, pair< string, int > >::const_iterator it = found_nums.begin(); it != found_nums.end(); ++it )
	{
		const int cur_val = it->second.second;
		if ( cur_val != -1 )
		{
			if ( best_val == -1
				|| found_nums[ ret ].second < cur_val )
			{
				ret = it->first;
				best_val = cur_val;
			}
		}
	}
	return ret;
}

int find_next_level( map< int, pair< string, int > >& found_nums )
{
	const int best_level = fine_best_level( found_nums );
	assert( best_level != -1 );
	// первое
	if ( best_level == found_nums.begin()->first )
	{
		map< int, pair< string, int > >::const_iterator it = found_nums.begin();
		++it;
		if ( it->second.second == -1 )
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
	map< int, pair< string, int > >::const_iterator it_end = found_nums.end();
	--it_end;
	if ( best_level == it_end->first )
	{
		// предпоследнее
		--it_end;
		if ( it_end->second.second == -1 )
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
	map< int, pair< string, int > >::const_iterator it_best = found_nums.find( best_level );
	map< int, pair< string, int > >::const_iterator it_prev = --found_nums.find( best_level );
	map< int, pair< string, int > >::const_iterator it_next = ++found_nums.find( best_level );
	// проверяем что нашли на двух шагах одно и тоже
	if ( it_best->second.first == it_prev->second.first
		|| it_best->second.first == it_next->second.first )
	{
		return -1;
	}
	// если с двух сторо найдено, то ничего не ищем больше
	if ( it_next->second.second != -1
		&& it_prev->second.second != -1 )
	{
		return -1;
	}
	// если с двух сторон пусто, то идем вниз
	if ( it_next->second.second == -1
		&& it_prev->second.second == -1 )
	{
		return it_prev->first;
	}
	// идем в ту сторону, где не найдено
	if ( it_prev->second.second == -1 )
	{
		return it_prev->first;
	}
	if ( it_next->second.second == -1 )
	{
		return it_next->first;
	}
	assert( !"тут не должны быть" );
	return -1;
}

std::pair< std::string, int > read_number_loop( const cv::Mat& image, map< int, pair< string, int > >& found_nums, recog_debug_callback *recog_debug )
{
	int next_level = find_next_level( found_nums );
	while ( next_level != -1 )
	{
		const vector< pair< string, int > > cur_nums = read_number( image, next_level, recog_debug );
		const pair< string, int > best_num = find_best( cur_nums );
		found_nums[ next_level ] = best_num;

		next_level = find_next_level( found_nums );
	}

	const int best_level = fine_best_level( found_nums );
	assert( best_level != -1 );
	return found_nums[ best_level ];
}

std::pair< std::string, int > read_number( const cv::Mat& image, recog_debug_callback *recog_debug )
{
	assert( recog_debug );
	map< int, pair< string, int > > found_nums;
	found_nums[ 127 ] = make_pair( string(), -1 );
	found_nums[ 63  ] = make_pair( string(), -1 );
	found_nums[ 191 ] = make_pair( string(), -1 );
	found_nums[ 31  ] = make_pair( string(), -1 );
	found_nums[ 95  ] = make_pair( string(), -1 );
	found_nums[ 159 ] = make_pair( string(), -1 );
	found_nums[ 223 ] = make_pair( string(), -1 );
	found_nums[ 15  ] = make_pair( string(), -1 );
	found_nums[ 47  ] = make_pair( string(), -1 );
	found_nums[ 79  ] = make_pair( string(), -1 );
	found_nums[ 111 ] = make_pair( string(), -1 );
	found_nums[ 143 ] = make_pair( string(), -1 );
	found_nums[ 175 ] = make_pair( string(), -1 );
	found_nums[ 207 ] = make_pair( string(), -1 );
	found_nums[ 239 ] = make_pair( string(), -1 );

	vector< pair< string, int > > all_nums;
	vector< int > first_search_levels;
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
	for ( size_t nn = 0; nn < first_search_levels.size(); ++nn )
	{
		const vector< pair< string, int > > cur_nums = read_number( image, first_search_levels.at( nn ), recog_debug );
		const pair< string, int > best_num = find_best( cur_nums );
		found_nums[ first_search_levels.at( nn ) ] = best_num;
		if ( best_num.second != 0 )
		{
			return read_number_loop( image, found_nums, recog_debug );
		}
	}
	return make_pair( std::string( "" ), 0 );
}

std::pair< std::string, int > read_number_by_level( const cv::Mat& image, int gray_level, recog_debug_callback *recog_debug )
{
	return find_best( read_number( image, gray_level, recog_debug ) );
}

std::vector< std::pair< std::string, int > > read_number( const cv::Mat& input, int grey_level, recog_debug_callback *recog_debug )
{
	Mat gray( input.size(), CV_8U );
	cvtColor( input, gray, CV_RGB2GRAY );
//	recog_debug->out_image( gray );
	Mat img_bw = gray > grey_level;
//	recog_debug->out_image( img_bw );
/*		// вырезаем одиночные пиксели (пока не нужно)
	for ( int nn = 0; nn < img_bw.rows; ++nn )
	{
		for ( int mm = 0; mm < img_bw.cols - 2; ++mm )
		{
			unsigned char c1 = img_bw.at< unsigned char >( nn, mm );
			unsigned char c2 = img_bw.at< unsigned char >( nn, mm + 1 );
			unsigned char c3 = img_bw.at< unsigned char >( nn, mm + 2 );
			if ( c1 == c3 && c2 != c1 )
			{
				img_bw.at< unsigned char >( nn, mm + 1 ) = c1;
			}
		}
	}
	for ( int nn = 0; nn < img_bw.cols; ++nn )
	{
		for ( int mm = 0; mm < img_bw.rows - 2; ++mm )
		{
			unsigned char c1 = img_bw.at< unsigned char >( mm, nn );
			unsigned char c2 = img_bw.at< unsigned char >( mm + 1, nn );
			unsigned char c3 = img_bw.at< unsigned char >( mm + 2, nn );
			if ( c1 == c3 && c2 != c1 )
			{
				img_bw.at< unsigned char >( mm + 1, nn ) = c1;
			}
		}
	}
	recog_debug->out_image( img_bw );*/
	cv::Mat img_to_rez = img_bw.clone();
	vector< std_figure > figs;
	figs.reserve( 1000 );
	//надо увеличить скорость разбития картинки на фигуры
	// бьем картинку на фигуры
	for ( int nn = 0; nn < img_bw.rows; ++nn )
	{
		for ( int mm = 0; mm < img_bw.cols; ++mm )
		{
			if ( img_bw.at< unsigned char >( nn, mm ) == 0 )
			{
				std_figure fig_to_create;
				add_pixel_as_spy( nn, mm, img_bw, fig_to_create );
				if ( fig_to_create.is_valid() )
				{
					// проверяем что высота больше ширины
					if ( fig_to_create.width() < fig_to_create.height() )
					{
						if ( fig_to_create.width() > 4 )
						{
							if ( fig_to_create.height() / fig_to_create.width() < 4 )
							{
								figs.push_back( fig_to_create );
							}
						}
					}
				}
			}
		}
	}
	// отрисовываем найденные фигуры
	if ( !figs.empty() )
	{
		Mat colored_rect_1( input.size(), CV_8UC3 );
		Mat colored_rect_2( input.size(), CV_8UC3 );
		cvtColor( img_to_rez, colored_rect_1, CV_GRAY2RGB );
		cvtColor( img_bw, colored_rect_2, CV_GRAY2RGB );
		for ( size_t nn = 0; nn < figs.size(); ++nn )
		{
			cv::rectangle( colored_rect_1, Point( figs[ nn ].left(), figs[ nn ].top() ), Point( figs[ nn ].right(), figs[ nn ].bottom() ), CV_RGB( 0, 255, 0 ) );
			cv::rectangle( colored_rect_2, Point( figs[ nn ].left(), figs[ nn ].top() ), Point( figs[ nn ].right(), figs[ nn ].bottom() ), CV_RGB( 0, 255, 0 ) );
		}
		recog_debug->out_image( colored_rect_2 );
		recog_debug->out_image( colored_rect_1 );
	}
	return procces_found_figs( figs, img_to_rez, recog_debug );
}
