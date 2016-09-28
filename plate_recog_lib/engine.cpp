#include "engine.h"
#include <assert.h>
#include <functional>
#include <stdexcept>

#include <opencv2/opencv.hpp>
#include <baseapi.h>
//#include <leptonica/allheaders.h>

#include "sym_recog.h"
#include "utils.h"
#include "figure.h"
#include "binarizewolfjolion.h"

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
	{ std::make_pair( 0, 1 ), std::make_pair( - 1, 0 ), std::make_pair( 1, 0 ), std::make_pair( 0, - 1 ) },
	{ std::make_pair( 0, 1 ), std::make_pair( - 1, 0 ), std::make_pair( 1, 0 ), std::make_pair( 0, - 1 ) },
	{ std::make_pair( 0, 1 ), std::make_pair( - 1, 0 ), std::make_pair( 1, 0 ), std::make_pair( 0, - 1 ) },
	{ std::make_pair( 0, 1 ), std::make_pair( - 1, 0 ), std::make_pair( 1, 0 ), std::make_pair( 0, - 1 ) },
	{ std::make_pair( 0, 1 ), std::make_pair( - 1, 0 ), std::make_pair( 1, 0 ), std::make_pair( 0, - 1 ) },
	{ std::make_pair( 0, 1 ), std::make_pair( - 1, 0 ), std::make_pair( 1, 0 ), std::make_pair( 0, - 1 ) },
	{ std::make_pair( 0, 1 ), std::make_pair( - 1, 0 ), std::make_pair( 1, 0 ), std::make_pair( 0, - 1 ) },
	{ std::make_pair( 0, 1 ), std::make_pair( - 1, 0 ), std::make_pair( 1, 0 ), std::make_pair( 0, - 1 ) } };

#ifdef _WIN32

#include <windows.h>

static bool g_busy_indexes[ COUNT_GLOBAL_DATA ] = { false, false, false, false, false, false, false, false };
HANDLE h_data_guard = CreateMutex( NULL, FALSE, NULL );
int get_free_index()
{
	if ( ::WaitForSingleObject( h_data_guard, INFINITE ) != WAIT_OBJECT_0 )
	{
		throw std::runtime_error( "failed call winapi func" );
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
	throw std::runtime_error( "no free data" );
}

void set_free_index( int index )
{
	assert( index >= 0 && index < COUNT_GLOBAL_DATA );
	if ( WaitForSingleObject( h_data_guard, INFINITE ) != WAIT_OBJECT_0 )
	{
		throw std::runtime_error( "failed call winapi func" );
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

const int kMinGroupSize = 4;

FoundNumber read_number_loop( const cv::Mat& input, const std::vector< int >& search_levels );

inline pair_int operator - ( const pair_int& lh, const pair_int& rh )
{
	return std::make_pair( lh.first - rh.first, lh.second - rh.second );
}

inline pair_int operator + ( const pair_int& lh, const pair_int& rh )
{
	return std::make_pair( lh.first + rh.first, lh.second + rh.second );
}

inline pair_int operator * ( const pair_int& lh, const double& koef )
{
	return std::make_pair( static_cast< int >( static_cast< double >( lh.first ) * koef ), static_cast< int >( static_cast< double >( lh.second ) * koef ) );
}

inline pair_int operator / ( const pair_int& lh, const int& koef )
{
	return std::make_pair( lh.first / koef, lh.second / koef );
}

inline pair_doub operator * ( const pair_doub& lh, const double& koef )
{
	return std::make_pair( static_cast< double >( lh.first * koef ), static_cast< double >( lh.second * koef ) );
}

// Рассчитываем центры символов
std::vector< pair_int > calc_syms_centers( int index, int angle, int first_fig_height )
{
	const double angl_grad = static_cast< double >( std::abs( angle < 0 ? angle + 10 : angle ) ) * 3.14 / 180.;
	const double koef = static_cast< double >( first_fig_height ) * cos( angl_grad );
	const double move_by_y = koef * 0.2;
	double move_by_x = 0.;
	std::vector< pair_int > etalons;
	if ( angle >= 0 )
	{
		move_by_x = koef * 0.96;
	}
	else
	{
		move_by_x = koef * 1.06;
	}
	etalons.push_back( std::make_pair( static_cast< int >( 1. * move_by_x ), -1 * static_cast< int >( move_by_y ) ) );
	etalons.push_back( std::make_pair( static_cast< int >( 1. * move_by_x ), -1 * static_cast< int >( move_by_y ) ) );
	etalons.push_back( std::make_pair( static_cast< int >( 1. * move_by_x ), -1 * static_cast< int >( move_by_y ) ) );
	etalons.push_back( std::make_pair( static_cast< int >( 1. * move_by_x ), 0 ) );
	etalons.push_back( std::make_pair( static_cast< int >( 1. * move_by_x ), 0 ) );

	std::vector< pair_int > ret;
	const pair_int ret_front( std::make_pair( 0, 0 ) );
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

std::pair< char, double > find_sym_nn( bool num, const Figure& fig, const cv::Mat& original, number_data& stat_data )
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

	std::map< std::pair< cv::Rect, bool >, std::pair< char, double >  >::const_iterator it = stat_data.m_cache_rets.find( std::make_pair( sym_border, num ) );
	if ( it != stat_data.m_cache_rets.end() )
	{
		return it->second;
	}

	cv::Mat mm( original.clone(), sym_border );
	std::pair< char, double >  ret = num ? proc_num( mm ) : proc_char( mm );
	stat_data.m_cache_rets[ std::make_pair( sym_border, num ) ] = ret;
	return ret;
}

// выбираем пиксели что бы получить контур, ограниченный белыми пикселями
template< int stat_data_index >
void add_pixel_as_spy( int row, int col, cv::Mat& mat, Figure& fig, int top_border = -1, int bottom_border = -1 )
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

bool fig_less_left( const Figure& lf, const Figure& rf )
{
	return lf.left() < rf.left();
}

bool less_by_left_pos( const Figure& lf, const Figure& rf )
{
	return fig_less_left( lf, rf );
}

void draw_figures( const FigureGroup& figs, const cv::Mat& etal, const std::string& key = "figs" )
{
	cv::Mat colored_rect( etal.size(), CV_8UC3 );
	cvtColor( etal, colored_rect, CV_GRAY2RGB );
	for ( size_t mm = 0; mm < figs.size(); ++mm )
	{
		const Figure* cur_fig = &figs.at( mm );
		rectangle( colored_rect, cv::Point( cur_fig->left(), cur_fig->top() ), cv::Point( cur_fig->right(), cur_fig->bottom() ), CV_RGB( 0, 255, 0 ) );
	}
	imwrite( next_name( key ), colored_rect );
}

Figure find_figure( const FigureGroup& gr, const pair_int& pos )
{
	for ( size_t nn = 0; nn < gr.size(); ++nn )
	{
		if ( gr[ nn ].top_left() <= pos
			&& gr[ nn ].bottom_right() >= pos )
		{
			return gr[ nn ];
		}
	}
	return Figure();
}

// Выкидываем группы, которые включают в себя другие группы
void RemoveIncludedGroups(std::vector<FigureGroup>& groups ) {
  // todo: переписать постоянное создание set
  bool merge_was = true;
  while (merge_was) {
    merge_was = false;
    for (size_t nn = 0; nn < groups.size() && !merge_was; ++nn) {
      const std::set<Figure> s_cur_f = to_set(groups[nn]);
      for (size_t mm = 0; mm < groups.size() && !merge_was; mm++) {
        if (nn != mm) {
          const std::set<Figure> s_test_f = to_set( groups[mm]);
          if (includes(s_cur_f.begin(), s_cur_f.end(), s_test_f.begin(), s_test_f.end())) {
            // нашли совпадение
            groups.erase(groups.begin() + mm);
            merge_was = true;
            break;
          }
        }
      }
    }
  }
}

// сливаем пересекающиеся группы
void MergeIntersectsGroups(std::vector<FigureGroup>& groups) {
  bool merge_was = true;
  while ( merge_was ) {
    merge_was = false;
    for (size_t nn = 0; nn < groups.size() && !merge_was; ++nn) {
      const std::set<Figure> s_cur_f = to_set(groups[nn]);
      for (size_t mm = 0; mm < groups.size() && !merge_was; ++mm) {
        if (nn != mm) {
          std::set<Figure> s_test_f = to_set(groups[mm]);
          if (find_first_of(s_test_f.begin(), s_test_f.end(), s_cur_f.begin(), s_cur_f.end()) != s_test_f.end()) {
            // нашли пересечение
            std::set<Figure> ss_nn = to_set(groups[nn]);
            const std::set< Figure > ss_mm = to_set(groups[mm]);
            ss_nn.insert(ss_mm.begin(), ss_mm.end());
            std::vector< Figure > res;
            for (std::set<Figure>::const_iterator it = ss_nn.begin(); it != ss_nn.end(); ++it) {
              res.push_back(*it);
            }
            groups[nn] = res;
            groups.erase(groups.begin() + mm);
            merge_was = true;
            break;
          }
        }
      }
    }
  }
}

void RemoveToSmallGroups(std::vector<FigureGroup>& groups) {
  for (int nn = static_cast<int>(groups.size()) - 1; nn >= 0; --nn) {
    if (groups[nn].size() < kMinGroupSize) {
      groups.erase( groups.begin() + nn );
    }
  }
}

// выкидываем элементы, что выходят за размеры номера, предпологаем что номер не выше 3 * высота первого элемента
void figs_remote_too_long_by_y_from_first( std::vector< FigureGroup > & groups )
{
	for ( size_t nn = 0; nn < groups.size(); ++nn )
	{
		if ( groups[ nn ].size() > 2 )
		{
			const Figure& first_fig = groups[ nn ].at( 0 );
			const double height_first = first_fig.height();
			assert( first_fig.height() > 0. );
			for ( int mm = groups[ nn ].size() - 1; mm >= 1; --mm )
			{
				const Figure& cur_fig = groups[ nn ][ mm ];
				if ( abs( double( cur_fig.top() - first_fig.top() ) / 3. ) > height_first )
				{
					groups[ nn ].erase( groups[ nn ].begin() + mm );
				}
			}
		}
	}
	// выкидываем группы, где меньше 3-х элементов
        RemoveToSmallGroups(groups);
}

// выкидываем элементы, что выходят за размеры номера относительно первого элемента
void RemoteTooFarFromFirst(std::vector<FigureGroup>& groups, int max_dist_coef_to_hei, std::function<int(const Figure&)> get_pos) {
  for (size_t nn = 0; nn < groups.size(); ++nn) {
    std::vector<FigureGroup>::iterator cur_group_iter = groups.begin() + nn;
    assert(cur_group_iter->size() >= 3);
    const Figure& first_fig = cur_group_iter->at(0);
    const int max_dist = first_fig.height() * max_dist_coef_to_hei;
    assert(first_fig.height() > 0.);
    for (int mm = cur_group_iter->size() - 1; mm >= 1; --mm) {
      const Figure& cur_fig = cur_group_iter->at(mm);
      if (get_pos(cur_fig) - get_pos(first_fig) > max_dist) {
        // @todo: тут можно оптимизировать и в начале найти самую допустимо-дальнюю, а уже затем чистить список
        cur_group_iter->erase(cur_group_iter->begin() + mm);
      } else {
        // тут брик, т.к. фигуры отсортированы по оси х и все предыдущие будут левее
        break;
      }
    }
  }
  // выкидываем группы, где меньше 3-х элементов
  RemoveToSmallGroups(groups);
}

void RemoveNotFitToFirstByHeight( std::vector< FigureGroup > & groups ) {
  for (size_t nn = 0; nn < groups.size(); ++nn) {
    std::vector<FigureGroup>::iterator cur_group_iter = groups.begin() + nn;
    const Figure& first_fig = cur_group_iter->at(0);
    const double height_first = static_cast<double>(first_fig.bottom() - first_fig.top());
    for (int mm = cur_group_iter->size() - 1; mm >= 1; --mm) {
      const Figure& cur_fig = cur_group_iter->at(mm);
      const double height_cur = cur_fig.bottom() - cur_fig.top();
      if (height_cur > height_first * 1.2 || height_cur < height_first * 0.8) {
        cur_group_iter->erase(cur_group_iter->begin() + mm);
      }
    }
  }

  RemoveToSmallGroups(groups);
}

// по отдельным фигурам создаем группы фигур
std::vector<FigureGroup> MakeGroupsByAngle(std::vector<Figure>& figs) {
  std::vector<FigureGroup> result;

  // формируем группы по углу
  for (size_t nn = 0; nn < figs.size(); ++nn) {
    std::vector<std::pair<double, FigureGroup>> cur_fig_groups;
    // todo: тут возможно стоит идти только от nn + 1, т.к. фигуры отсортированы и идем только вправо
    for (size_t mm = 0; mm < figs.size(); ++mm) {
      if (mm != nn) {
        if (figs[mm].left() > figs[nn].left()) {
	  // угол между левыми-нижними углами фигуры
	  const double angle = 57.2957795 * atan2(static_cast<double>(figs[mm].left() - figs[nn].left()), static_cast<double>(figs[mm].bottom() - figs[nn].bottom()));
	  bool found = false;
          for (size_t kk = 0; kk < cur_fig_groups.size(); ++kk) {
            // проверяем что попадает в группу
            if (angle_is_equal(static_cast<int>(cur_fig_groups[kk].first), static_cast<int>(angle))) {
              bool ok = true;
              // проверяем что бы угол был такой же как и у всех елементов группы, что бы не было дуги или круга
              for (size_t yy = 1; yy < cur_fig_groups[kk].second.size(); ++yy) {
                const Figure& next_fig = cur_fig_groups[kk].second.at(yy);
		const double angle_to_fig = 57.2957795 * atan2(static_cast<double>(figs[mm].left() - next_fig.left()), static_cast<double>(figs[mm].bottom() - next_fig.bottom()));
                if (!angle_is_equal(static_cast<int>(cur_fig_groups[kk].first), static_cast<int>(angle_to_fig))) {
                  ok = false;
                  break;
                }
              }
              if (ok) {
                cur_fig_groups[kk].second.push_back(figs[mm]);
                found = true;
                break;
              }
            }
          }

          // создаем группу
          if ( !found ) {
            FigureGroup to_add;
            to_add.push_back( figs[ nn ] );
            to_add.push_back( figs[ mm ] );
            cur_fig_groups.push_back( std::make_pair( angle, to_add ) );
          }
        }
      }
    }

    // проверяем что бы элементов в группе было больше 3-х
    for (size_t mm = 0; mm < cur_fig_groups.size(); ++mm) {
      if (cur_fig_groups[mm].second.size() >= kMinGroupSize) {
        // сортируем фигуры в группах, что бы они шли слева направо
        sort(cur_fig_groups[mm].second.begin(), cur_fig_groups[mm].second.end(), fig_less_left);
        result.push_back(cur_fig_groups[mm].second);
      }
    }
  }

  // выкидываем все группы, у которых угол больше 30 градусов
  for ( int nn = static_cast<int>(result.size()) - 1; nn >= 0; --nn) {
    const double xx = abs(static_cast<double>(result.at(nn).at(0).left() - result.at(nn).at(1).left()));
    const double yy = abs( static_cast<double>(result.at(nn).at(0).bottom() - result.at(nn).at(1).bottom()));
    const double angle_to_fig = abs(57.2957795 * atan2(yy, xx));
    if (angle_to_fig > 30. && angle_to_fig < 330.) {
      result.erase(result.begin() + nn);
    }
  }

  return result;
}

struct found_symbol
{
  Figure fig;
	size_t pos_in_pis_index;
	char symbol;
	double weight;
};

FoundNumber find_best_number_by_weight( const std::vector< FoundNumber >& vals, const cv::Mat* etal = 0 )
{
	(void)etal;
	assert( !vals.empty() );
	if ( vals.empty() )
		return FoundNumber();
	size_t best_index = 0;
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
			cv::Mat colored_rect( etal->size(), CV_8UC3 );
			cvtColor( *etal, colored_rect, CV_GRAY2RGB );
			for ( size_t nn = 0; nn < vals[ best_index ].figs.size(); ++nn )
			{
				const Figure* cur_fig = &vals[ best_index ].figs.at( nn );
				rectangle( colored_rect, Point( cur_fig->left(), cur_fig->top() ), Point( cur_fig->right(), cur_fig->bottom() ), CV_RGB( 0, 255, 0 ) );
			}
			imwrite( next_name( "fig" ), colored_rect );
		}
		tt++;*/
	}
	return vals[ best_index ];
}

FoundNumber create_number_by_pos( const std::vector< pair_int >& pis, const std::vector< found_symbol >& figs_by_pos )
{
	assert( pis.size() == 6 );
        FoundNumber ret;
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

std::vector< found_symbol > figs_search_syms( const std::vector< pair_int >& pis, const pair_int& pos_center, const FigureGroup& cur_gr, const cv::Mat& original, number_data& stat_data )
{
//	draw_figures( cur_gr, etal );
	std::vector< found_symbol > ret;
	std::set<Figure> procs_figs;
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
			const std::pair< char, double > cc = find_sym_nn( kk >= 1 && kk <= 3, next.fig, original, stat_data );
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

// @todo (sploid): refactor for support output with several symbols
std::pair<char, int> RecognizeSymbol(const cv::Mat& img) {
  static tesseract::TessBaseAPI* api = nullptr;
  if (!api) {
    api = new tesseract::TessBaseAPI();
    const int init_res = api->Init("C:\\soft\\alpr\\tess-install\\tessdata", "eng");
    api->SetVariable("tessedit_char_whitelist", "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
    api->SetVariable("save_blob_choices", "T");
    api->SetPageSegMode(tesseract::PSM_SINGLE_CHAR);
  }
  //  api->SetImage(etal.data, etal.size().width, etal.size().height, etal.channels(), etal.step1());

  //    api->SetRectangle(next.left(), next.top(), next.width(), next.height());
  api->SetImage(img.data, img.size().width, img.size().height, img.channels(), img.step1());
  const int rec_result = api->Recognize(NULL);

  tesseract::ResultIterator* ri = api->GetIterator();
  tesseract::PageIteratorLevel level = tesseract::RIL_SYMBOL;
  std::pair<char, int> res = std::make_pair(FoundNumber::kUnknownSym, 0);
  if (ri) {
    const char* symbol = ri->GetUTF8Text(level);
    if (symbol) {
      if (*symbol != ' ') {
        const float conf = ri->Confidence(level);
        res = std::make_pair(*symbol, static_cast<int>(conf * 10));
      }
      delete[] symbol;
    }
  }
  return res;
}

FoundNumber RecognizeNumberByGroup(cv::Mat& etal, const FigureGroup& group, const cv::Mat& original, number_data& stat_data) {
  FoundNumber result;

  for (const auto& next : group) {
    const cv::Mat copy = etal(cv::Rect(next.left(), next.top(), next.width() + 1, next.height() + 1)).clone();
    const std::pair<char, int> rec_res = RecognizeSymbol(copy);
    result.m_number += rec_res.first;
    result.m_weight += rec_res.second;
    result.m_figs.push_back(next);
  }

  assert(result.m_number.size() == group.size());

  return result;
}

std::vector<FoundNumber> RecognizeNumber(cv::Mat& etal, std::vector<FigureGroup>& groups, const cv::Mat& original, number_data& stat_data) {
  // ищем позиции фигур и соответсвующие им символы
  std::vector<FoundNumber> ret;
  for (size_t nn = 0; nn < groups.size(); ++nn) {
    const FoundNumber fn = RecognizeNumberByGroup(etal, groups.at(nn), original, stat_data);
    if (fn.is_valid()) {
      ret.push_back(fn);
    }


/*    std::vector<found_number> fig_nums_sums;
    const FigureGroup& cur_gr = groups[nn];
    // перебираем фигуры, подставляя их на разные места (пока перебираем только фигуры 0-1-2)
    for (size_t mm = 0; mm < 2; ++mm) { // todo: закоментил перебор первого символа
      const Figure& cur_fig = cur_gr[mm];
      const pair_int cen = cur_fig.center();
      // подставляем текущую фигуру на все позиции (пока ставим только первую и вторую фигуру)
      for (int ll = 0; ll < 1; ++ll) {
        // меняем угол наклона номера относительно нас (0 - смотрим прям на номер, если угол меньше 0, то определяем номер с 2-х значным регионом)
        for (int oo = -60; oo < 50; oo += 10) {
          const std::vector<pair_int> pis = calc_syms_centers(ll, oo, cur_fig.height());
          assert(pis.size() == 6); // всегда 6 символов без региона в номере
          const std::vector< found_symbol > figs_by_pos = figs_search_syms( pis, cen, cur_gr, original, stat_data );
          const found_number number_sum = create_number_by_pos( pis, figs_by_pos );
          fig_nums_sums.push_back( number_sum );
        }
      }
    }

    // выбираем лучшее
    if (!fig_nums_sums.empty()) {
      ret.push_back( find_best_number_by_weight(fig_nums_sums, &etal) );
    }*/
  }

  return ret;
}

int fine_best_index(const std::vector<FoundNumber>& found_nums) {
  if (found_nums.empty()) {
    return -1;
  } else if (found_nums.size() == 1U) {
    return 0;
  }
  int ret = 0;
  for (size_t nn = 1; nn < found_nums.size(); ++nn) {
    if (found_nums.at(ret) < found_nums.at(nn)) {
      ret = static_cast<int>(nn);
    }
  }
  return ret;
}

cv::Mat create_gray_image( const cv::Mat& input )
{
	cv::Mat gray( input.size(), CV_8U );
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
		std::cout << "!!! Invalid chanel count: " << input.channels() << " !!!" << std::endl;
		assert( !"не поддерживаемое количество каналов" );
	}
	return gray;
}

// бьем картинку на фигуры
template<int stat_data_index>
FigureGroup ParseToFigures(cv::Mat& mat) {
// time_mesure to_fig( "TO_FIGS: " );
  FigureGroup ret;
  ret.reserve(1000);
  for (int nn = 0; nn < mat.rows; ++nn) {
    for (int mm = 0; mm < mat.cols; ++mm ) {
      if (mat.at<unsigned char>(nn, mm) == 0) {
        Figure fig_to_create;
        add_pixel_as_spy<stat_data_index>(nn, mm, mat, fig_to_create);
        if (fig_to_create.is_too_big()) {
           // проверяем что высота больше ширины
          if (fig_to_create.width() < fig_to_create.height()) {
            if (fig_to_create.width() > 1) {
//              if (fig_to_create.height() / fig_to_create.width() < 4) {
                ret.push_back( fig_to_create );
///	      }
	    }
          }
        }
      }
    }
  }
	// отрисовываем найденные фигуры
/*	if ( !ret.empty() )
	{
		cv::Mat colored_mat( mat.size(), CV_8UC3 );
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

void fill_reg( std::vector< std::vector< pair_doub > >& data, double x1, double y1, double x2, double y2, double x3, double y3 )
{
	std::vector< pair_doub > tt;
	tt.push_back( std::make_pair( x1, y1 ) );
	tt.push_back( std::make_pair( x2, y2 ) );
	tt.push_back( std::make_pair( x3, y3 ) );
	data.push_back( tt );
}

void fill_reg( std::vector< std::vector< pair_doub > >& data, double x1, double y1, double x2, double y2 )
{
	std::vector< pair_doub > tt;
	tt.push_back( std::make_pair( x1, y1 ) );
	tt.push_back( std::make_pair( x2, y2 ) );
	data.push_back( tt );
}

// ищем ближайшую черную точку
pair_int search_nearest_black( const cv::Mat& etal, const pair_int& center )
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
	bool operator==( const Figure& other ) const
	{
		return m_fig == other;
	}
	char m_symbol;
	double m_weight;
        Figure m_fig;
};

template< int stat_data_index >
void search_region_symbol(FoundNumber& number, const cv::Mat& etal, const cv::Mat& origin, const pair_int& reg_center, const double avarage_height, bool last_symbol, number_data& stat_data )
{
//	number.m_figs.push_back( Figure( reg_center, std::make_pair( 1, 1 ) ) );
	const pair_int nearest_black = search_nearest_black( etal, reg_center );
//	number.m_figs.push_back( Figure( nearest_black, std::make_pair( 1, 1 ) ) );
//	return;

	if ( nearest_black.first != -1
		&& nearest_black.second != -1 )
	{
          Figure top_border_fig;
          Figure conture_fig;
		cv::Mat to_search = etal.clone();
		// Ищем контуры по верхней границе
		add_pixel_as_spy< stat_data_index >( nearest_black.second, nearest_black.first, to_search, top_border_fig, -1, nearest_black.second + 1 );
		if ( top_border_fig.top() > reg_center.second - static_cast< int >( avarage_height ) ) // ушли не далее чем на одну фигуру
		{
			cv::Mat to_contur = etal.clone();
			add_pixel_as_spy< stat_data_index >( nearest_black.second, nearest_black.first, to_contur, conture_fig, -1, top_border_fig.top() + static_cast< int >( avarage_height ) + 1 );
			if ( conture_fig.width() >= static_cast< int >( avarage_height ) ) // если широкая фигура, то скорее всего захватили рамку
			{
				// центрируем фигуру по Х (вырезаем не большой кусок и определяем его центр)
				cv::Mat to_stable = etal.clone();
                                Figure short_fig;
				// если центр сильно уехал, этот фокус не сработает, т.к. мы опять захватим рамку
				add_pixel_as_spy< stat_data_index >( nearest_black.second, nearest_black.first, to_stable, short_fig, nearest_black.second - static_cast< int >( avarage_height * 0.4 ), nearest_black.second + static_cast< int >( avarage_height * 0.4 ) );
				conture_fig = Figure( pair_int( short_fig.center().first, reg_center.second ), pair_int( static_cast< int >( avarage_height * 0.60 ), static_cast< int >( avarage_height ) ) );
			}
			else if ( last_symbol && conture_fig.width() >= static_cast< int >( avarage_height * 0.80 ) ) // если последняя фигура, то могли захватить болтик черный
			{
				// Тут делаю обработку захвата болтика
				conture_fig = Figure( conture_fig.left(), conture_fig.left() + static_cast< int >( avarage_height * 0.60 ), conture_fig.top(), conture_fig.bottom() );
			}
		}
		else
		{
			// центрируем фигуру по Х (вырезаем не большой кусок и определяем его центр)
			cv::Mat to_stable = etal.clone();
                        Figure short_fig;
			// если центр сильно уехал, этот фокус не сработает, т.к. мы опять захватим рамку
			add_pixel_as_spy< stat_data_index >( nearest_black.second, nearest_black.first, to_stable, short_fig, nearest_black.second - static_cast< int >( avarage_height * 0.4 ), nearest_black.second + static_cast< int >( avarage_height * 0.4 ) );
			if ( !short_fig.is_empty() )
			{
				conture_fig = Figure( pair_int( short_fig.center().first, reg_center.second ), pair_int( static_cast< int >( avarage_height * 0.60 ), static_cast< int >( avarage_height ) ) );
			}
			else
			{
				conture_fig = Figure();
			}
		}
		if ( !conture_fig.is_empty()
			&& find( number.m_figs.begin(), number.m_figs.end(), conture_fig ) == number.m_figs.end()
			&& static_cast< double >( conture_fig.height() ) > 0.5 * avarage_height
			&& static_cast< double >( conture_fig.width() ) > 0.2 * avarage_height
			)
		{
			const std::pair< char, double > sym_sym = find_sym_nn( true, conture_fig, origin, stat_data );
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

pair_int calc_center( const FigureGroup& figs, const std::vector< std::vector< pair_doub > >& data, int index )
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

void apply_angle( const FigureGroup& figs, std::vector< std::vector< pair_doub > >& data, double avarage_height, double sin_avarage_angle_by_y )
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

std::vector< std::vector< pair_doub > > get_2_sym_reg_koef( const FigureGroup& figs, double avarage_height, double sin_avarage_angle_by_y )
{
	std::vector< std::vector< pair_doub > > ret;
	fill_reg( ret, 6.7656, -0.4219, 7.5363, -0.3998 );
	fill_reg( ret, 5.6061, -0.2560, 6.3767, -0.2338 );
	fill_reg( ret, 4.6016, -0.2991, 5.3721, -0.2769 );
	fill_reg( ret, 3.5733, -0.3102, 4.3440, -0.2880 );
	fill_reg( ret, 2.4332, -0.4802, 3.2039, -0.4580 );
	fill_reg( ret, 1.4064, -0.5123, 2.1770, -0.4901 );
	apply_angle( figs, ret, avarage_height, sin_avarage_angle_by_y );
	return ret;
}

std::vector< std::vector< pair_doub > > get_3_sym_reg_koef( const FigureGroup& figs, double avarage_height, double sin_avarage_angle_by_y )
{
	std::vector< std::vector< pair_doub > > ret;
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
std::vector< sym_info > select_sym_info( const std::vector< sym_info >& fs2, const std::vector< sym_info >& fs3 )
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

bool compare_regions( const FoundNumber& lh, const FoundNumber& rh )
{
/*	const bool lh_in_reg_list = region_codes().find( lh.m_number ) != region_codes().end();
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
	}*/
	return false;
}

pair_int get_pos_next_in_region( const FigureGroup& figs, const std::vector< std::vector< pair_doub > >& move_reg, const int index, const FoundNumber& number, const double avarage_height )
{
	pair_int ret = calc_center( figs, move_reg, index );
	if ( number.m_number.at( number.m_number.size() - 1 ) != '?' )
	{
		return number.m_figs.at( number.m_figs.size() - 1 ).center() + std::make_pair( static_cast< int >( 0.75 * avarage_height ), 0 );
	}
	return ret;
}

template< int stat_data_index >
void search_region(FoundNumber& best_number, const int best_level, const cv::Mat& original, number_data& stat_data )
{
	const FigureGroup& figs = best_number.m_figs;
	if ( figs.size() != 6 ) // ищем регион только если у нас есть все 6 символов
	{
		return;
	}

	const double koef_height = 0.76; // отношение высоты буквы к высоте цифры
	// средняя высота буквы
	const double avarage_height = ( static_cast< double >( figs.at( 0 ).height() + figs.at( 4 ).height() + figs.at( 5 ).height() ) / 3.
		+ static_cast< double >( figs.at( 1 ).height() + figs.at( 2 ).height() + figs.at( 3 ).height() ) * koef_height / 3. ) / 2.;

	// ищем угол наклона по высоте (считаем среднее между не соседними фигурами 0-4, 0-5, 1-3)
	std::vector< pair_int > index_fig_to_angle;
	index_fig_to_angle.push_back( std::make_pair( 0, 4 ) );
	index_fig_to_angle.push_back( std::make_pair( 0, 5 ) );
	index_fig_to_angle.push_back( std::make_pair( 1, 3 ) );
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

	std::vector< int > levels_to_iterate;
	for ( int nn = best_level - 20; nn <= best_level + 20; nn += 10 )
	{
		levels_to_iterate.push_back( nn );
	}
	levels_to_iterate.erase( remove_if( levels_to_iterate.begin(), levels_to_iterate.end(), &not_in_char_distance ), levels_to_iterate.end() );
//	levels_to_iterate.push_back( best_level + 10 );

	std::vector< FoundNumber > nums;
	for ( size_t nn = 0; nn < levels_to_iterate.size(); ++nn )
	{
		cv::Mat etal = original > levels_to_iterate.at( nn );
		{
                  FoundNumber fs2;
			const std::vector< std::vector< pair_doub > > move_reg_by_2_sym_reg( get_2_sym_reg_koef( figs, avarage_height, sin_avarage_angle_by_y ) );
			search_region_symbol< stat_data_index >( fs2, etal, original, calc_center( figs, move_reg_by_2_sym_reg, 0 ), avarage_height, false, stat_data );
			const pair_int next_2 = get_pos_next_in_region( figs, move_reg_by_2_sym_reg, 1, fs2, avarage_height );
			search_region_symbol< stat_data_index >( fs2, etal, original, next_2, avarage_height, true, stat_data );
			nums.push_back( fs2 );
		}
		{
                  FoundNumber fs3;
			const std::vector< std::vector< pair_doub > > move_reg_by_3_sym_reg( get_3_sym_reg_koef( figs, avarage_height, sin_avarage_angle_by_y ) );
			search_region_symbol< stat_data_index >( fs3, etal, original, calc_center( figs, move_reg_by_3_sym_reg, 0 ), avarage_height, false, stat_data );
			const pair_int next_2 = get_pos_next_in_region( figs, move_reg_by_3_sym_reg, 1, fs3, avarage_height );
/*			pair_int next_2 = calc_center( figs, move_reg_by_3_sym_reg, 1 );
			if ( fs3.m_number.at( fs3.m_number.size() - 1 ) != '?' )
			{
				next_2 = fs3.m_figs.at( fs3.m_figs.size() - 1 ).center() + std::make_pair( static_cast< int >( 0.75 * avarage_height ), 0 );
			}*/
			search_region_symbol< stat_data_index >( fs3, etal, original, next_2, avarage_height, false, stat_data );
			const pair_int next_3 = get_pos_next_in_region( figs, move_reg_by_3_sym_reg, 2, fs3, avarage_height );
/*			pair_int next_3 = calc_center( figs, move_reg_by_3_sym_reg, 2 );
			if ( fs3.m_number.at( fs3.m_number.size() - 1 ) != '?' )
			{
				next_3 = fs3.m_figs.at( fs3.m_figs.size() - 1 ).center() + std::make_pair( static_cast< int >( 0.75 * avarage_height ), 0 );
			}*/
			search_region_symbol< stat_data_index >( fs3, etal, original, next_3, avarage_height, true, stat_data );
			nums.push_back( fs3 );
		}
	}
	sort( nums.begin(), nums.end(), &compare_regions );
	std::vector< FoundNumber >::const_iterator it = max_element( nums.begin(), nums.end(), &compare_regions );

	best_number.m_number.append( it->m_number );
	best_number.m_weight += it->m_weight;
	for ( size_t nn = 0; nn < it->m_figs.size(); ++nn )
	{
		best_number.m_figs.push_back( it->m_figs.at( nn ) );
	}

}

FoundNumber read_number(const cv::Mat& image, int gray_step) {
  if (gray_step <= 0 || gray_step >= 256)
    gray_step = 10;

  std::vector<int> search_levels;
  for (int nn = gray_step; nn < 255; nn += gray_step) {
    search_levels.push_back( nn );
  }

  return read_number_loop(image, search_levels);
}

FoundNumber read_number_by_level( const cv::Mat& image, int gray_level )
{
	std::vector< int > search_levels;
	if ( gray_level <= 0 || gray_level >= 256 )
		gray_level = 127;

	search_levels.push_back( gray_level );
	return read_number_loop( image, search_levels );
}

void remove_single_pixels( cv::Mat& mat )
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

void ReplaceBlackWhite(cv::Mat& input) {
  for (int nn = 0; nn < input.rows; ++nn) {
    for (int mm = 0; mm < input.cols; ++mm) {
      switch (input.at<unsigned char>(nn, mm)) {
        case 0:
          input.at<unsigned char>(nn, mm) = 255;
          break;
        case 255:
          input.at<unsigned char>(nn, mm) = 0;
          break;
        default:
          assert(!"invalid color");
          break;
      }
    }
  }
}

void RemoveTooNarrowFigures(FigureGroup& figs) {
  for (int nn = static_cast<int>(figs.size() - 1); nn >= 0; --nn) {
    assert(figs.at(nn).width() && figs.at(nn).height());
    const float diff = static_cast<float>(figs.at(nn).width()) / static_cast<float>(figs.at(nn).height());
    const float kMinDiff = 0.1;
    if (diff < kMinDiff) {
      figs.erase(figs.begin() + nn);
    }
  }
}

template<int stat_data_index>
std::vector<FoundNumber> ReadNumberImpl(const cv::Mat& input, int gray_level, number_data& stat_data) {
  const cv::Mat& gray_image = create_gray_image(input);
  cv::Mat img_bw = gray_image > gray_level;

  cv::Mat iii1 = img_bw.clone();
  NiblackSauvolaWolfJolion(gray_image, iii1, WOLFJOLION, gray_level / 10, gray_level / 10, 0.05);
//  cv::Mat iii2 = img_bw.clone();
//  NiblackSauvolaWolfJolion(gray_image, iii2, WOLFJOLION, 7, 7, 0.1);
//  cv::Mat iii3 = img_bw.clone();
//  NiblackSauvolaWolfJolion(gray_image, iii3, WOLFJOLION, 7, 7, 0.15);



  //ReplaceBlackWhite(img_bw); // конвертим для сингапура
  //cv::Mat img_to_recog = img_bw.clone();
  //FigureGroup figs = ParseToFigures<stat_data_index>(img_bw);

  ReplaceBlackWhite(iii1); // конвертим для сингапура
  cv::Mat img_to_recog = iii1.clone();
  FigureGroup figs = ParseToFigures<stat_data_index>(iii1);

//  cv::Mat mama = input.clone();
//  DrawFigures(mama, figs);


  // выкидываем слишком узкие штуки
  RemoveTooNarrowFigures(figs);
  std::vector<FigureGroup> groups = MakeGroupsByAngle(figs);
  // удаляем лишнее по ширине
  RemoteTooFarFromFirst(groups, 7, std::bind(&Figure::left, std::placeholders::_1));
  // удаляем лишнее по длине
  RemoteTooFarFromFirst(groups, 3, std::bind(&Figure::left, std::placeholders::_1));
  // выкидываем элементы, не пропорциональные первому элементу
  RemoveNotFitToFirstByHeight(groups);
  // выкидываем группы, которые включаются в другие группы
  RemoveIncludedGroups(groups);
  // сливаем пересекающиеся группы
  MergeIntersectsGroups(groups);
  cv::Mat deb = input.clone();
  DrawGroupsFigures(deb, groups, CV_RGB(255, 0, 0));
  // ищем номера
  const std::vector<FoundNumber> nums = RecognizeNumber(img_to_recog, groups, gray_image, stat_data);
  return nums;
}

FoundNumber read_number_loop(const cv::Mat& input, const std::vector<int>& search_levels) {
  std::vector<FoundNumber> all_nums;
  const int free_index = get_free_index();
  try {
    number_data stat_data;
    for (size_t nn = 0; nn < search_levels.size(); ++nn) {
      std::vector<FoundNumber> cur_nums;
      switch (free_index) {
        case 0: cur_nums = ReadNumberImpl<0>(input, search_levels.at(nn), stat_data); break;
        case 1: cur_nums = ReadNumberImpl<1>(input, search_levels.at(nn), stat_data); break;
        case 2: cur_nums = ReadNumberImpl<2>(input, search_levels.at(nn), stat_data); break;
        case 3: cur_nums = ReadNumberImpl<3>(input, search_levels.at(nn), stat_data); break;
        case 4: cur_nums = ReadNumberImpl<4>(input, search_levels.at(nn), stat_data); break;
        case 5: cur_nums = ReadNumberImpl<5>(input, search_levels.at(nn), stat_data); break;
        case 6: cur_nums = ReadNumberImpl<6>(input, search_levels.at(nn), stat_data); break;
        case 7: cur_nums = ReadNumberImpl<7>(input, search_levels.at(nn), stat_data); break;
	default: throw std::runtime_error( "invalid data index" );
      };

      if (!cur_nums.empty()) {
        all_nums.insert(all_nums.end(), cur_nums.begin(), cur_nums.end());
//        found_nums.push_back(cur_nums.at(fine_best_index(cur_nums)));
      }
    }

/*  if ( best_index != -1 )
		{
                  FoundNumber& best_number = found_nums.at( best_index );
			const cv::Mat& gray_image = create_gray_image( input );
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
				throw std::runtime_error( "invalid data index" );
			};

			ret = best_number;
		}*/
  } catch ( const cv::Exception& e) {
    const char* tt = e.what();
    int t = 0;
  }
  set_free_index(free_index);
  const int best_index = fine_best_index(all_nums);
  for (size_t nn = 0; nn < all_nums.size(); ++nn) {
    cv::Mat ll = input.clone();
    DrawFigures(ll, all_nums.at(nn).m_figs);
//    cv::putText(ll, all_nums.at(nn).m_number + "  " + std::to_string(all_nums.at(nn).m_weight), cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, CV_RGB(255, 0, 0), 2);
    std::stringstream ss;
    ss << "C:\\cache\\sing\\logs\\" << nn;
    if (best_index == nn) {
      ss << "b";
    }
    std::string num_with_star(all_nums.at(nn).m_number);
    std::replace(num_with_star.begin(), num_with_star.end(), FoundNumber::kUnknownSym, '-');
    ss << "  " << all_nums.at(nn).m_weight << "  " << num_with_star << ".jpg";
    cv::imwrite(ss.str(), ll);
  }
  if (best_index == -1) {
    return FoundNumber();
  } else {
    return all_nums.at(best_index);
  }
}
