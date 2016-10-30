#include "engine.h"
#include <algorithm>
#include <array>
#include <assert.h>
#include <functional>
#include <memory>
#include <numeric>
#include <stdexcept>

#include <opencv2/opencv.hpp>

#include "sym_recog.h"
#include "utils.h"
#include "figure.h"

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
const int kMaxDistByX = 8;
const int kMaxDistByY = 2;

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
bool AngleIsEqual(int an1, int an2) {
  const int kAngleDiff = 16;
  int an_max = an1 + kAngleDiff;
  int an_min = an1 - kAngleDiff;
  if (an_min >= 0 && an_max <= 360) {
    const bool ret = an2 < an_max && an2 > an_min;
    return ret;
  } else if (an_min <= 0) {
    an_min = an_min + 360;
    return an2 < an_max || an2 > an_min;
  } else if (an_max > 360) {
    an_max = an_max - 360;
    return an2 < an_max || an2 > an_min;
  }
  assert(false);
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

bool FigLessLeft(const Figure& lf, const Figure& rf) {
  return lf.left() < rf.left();
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

// выкидываем элементы, что выходят за размеры номера относительно первого элемента
void RemoteTooFarFromFirst(std::vector<FigureGroup>& groups, int max_dist_coef_to_hei, std::function<int(const Figure&)> get_pos) {
  for (auto cur_group_iter = begin(groups); cur_group_iter != end(groups); ++cur_group_iter) {
    assert(cur_group_iter->size() >= 3);
    const Figure& first_fig = cur_group_iter->at(0);
    const int max_dist = first_fig.height() * max_dist_coef_to_hei;
    assert(first_fig.height() > 0.);
    for (int mm = static_cast<int>(cur_group_iter->size()) - 1; mm >= 1; --mm) {
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

bool IsFitByHeight(const Figure& one, const Figure& two) {
  const double height_one = static_cast<double>(one.height());
  const double height_two = static_cast<double>(two.height());
  return height_two * 0.8 < height_one  && height_one < height_two * 1.2;
}

void RemoveNotFitToFirstByHeight(std::vector<FigureGroup>& groups) {
  for (auto cur_group_iter = begin(groups); cur_group_iter != end(groups); ++cur_group_iter) {
    for (int mm = static_cast<int>(cur_group_iter->size()) - 1; mm >= 1; --mm) {
      if (!IsFitByHeight(cur_group_iter->at(mm), cur_group_iter->at(0))) {
        cur_group_iter->erase(cur_group_iter->begin() + mm);
      }
    }
  }

  RemoveToSmallGroups(groups);
}

// проверяем что бы угол был такой же как и у всех елементов группы, что бы не было дуги или круга
bool IsFigFitToGroupAngle(const Figure& fig, const FigureGroup& group, int group_angle) {
  for (size_t yy = 1; yy < group.size(); ++yy) {
    const Figure& next_fig = group.at(yy);
    double angle_to_fig = 0.;
    if (fig.left() > next_fig.left()) {
      angle_to_fig = 57.2957795 * atan2(static_cast<double>(fig.left() - next_fig.left()), static_cast<double>(fig.bottom() - next_fig.bottom()));
    } else {
      angle_to_fig = 57.2957795 * atan2(static_cast<double>(next_fig.left() - fig.left()), static_cast<double>(next_fig.bottom() - fig.bottom()));
    }
    if (!AngleIsEqual(static_cast<int>(group_angle), static_cast<int>(angle_to_fig))) {
      return false;
    }
  }
  return true;
}

double CalculateAngle(const Figure& fig_left, const Figure& fig_right) {
  return 57.2957795 * atan2(static_cast<double>(fig_left.left() - fig_right.left()), static_cast<double>(fig_left.bottom() - fig_right.bottom()));
}

// подсчитываем средний угол в группе
double CalculateAngle(const FigureGroup& group) {
  assert(kMinGroupSize <= group.size());
  double result = 0.;
  size_t count = 0;
  for (size_t nn = 0; nn < group.size() - 1; ++nn) {
    for (size_t mm = nn + 1; mm < group.size(); ++mm) {
      result += CalculateAngle(group[mm], group[nn]);
      ++count;
    }
  }
  return result / count;
}

// по отдельным фигурам создаем группы фигур
std::vector<FigureGroup> MakeGroupsByAngle(std::vector<Figure>& figs) {
  std::vector<FigureGroup> result;

  // формируем группы по углу для фигуры nn
  for (size_t nn = 0; nn < figs.size(); ++nn) {
    std::vector<std::pair<double, FigureGroup>> cur_fig_groups;
    // todo: тут возможно стоит идти только от nn + 1, т.к. фигуры отсортированы и идем только вправо
    for (size_t mm = 0; mm < figs.size(); ++mm) {
      if (mm != nn) {
        const Figure& fig_to_add = figs[mm];
        if (fig_to_add.left() > figs[nn].left()) {
	  // угол между левыми-нижними углами фигуры
	  const double angle = CalculateAngle(fig_to_add, figs[nn]);
          std::vector<std::pair<double, FigureGroup>>::iterator cur_gr_iter = std::find_if(begin(cur_fig_groups), end(cur_fig_groups), [angle, fig_to_add](const std::pair<double, FigureGroup>& next_val) -> bool {
            return AngleIsEqual(static_cast<int>(next_val.first), static_cast<int>(angle))
                && IsFigFitToGroupAngle(fig_to_add, next_val.second, static_cast<int>(next_val.first));
          });

          if (cur_gr_iter != end(cur_fig_groups)) {
            cur_gr_iter->second.push_back(fig_to_add);
          } else {
            FigureGroup group_to_add;
            group_to_add.push_back(figs[nn]);
            group_to_add.push_back(fig_to_add);
            cur_fig_groups.push_back(std::make_pair(angle, group_to_add));
          }
        }
      }
    }

    // проверяем что бы элементов в группе было больше 3-х
    for (size_t mm = 0; mm < cur_fig_groups.size(); ++mm) {
      if (cur_fig_groups[mm].second.size() >= kMinGroupSize) {
        // сортируем фигуры в группах, что бы они шли слева направо
        sort(cur_fig_groups[mm].second.begin(), cur_fig_groups[mm].second.end(), FigLessLeft);
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
  static cv::Ptr<cv::ml::ANN_MLP> mlp;
  if (!mlp) {
    mlp = cv::ml::ANN_MLP::create();
    cv::FileStorage fs("C:\\cache\\sing\\all_syms_train.xml", cv::FileStorage::READ | cv::FileStorage::FORMAT_XML);
    mlp->read(fs.root());
  }

  cv::Mat to_pred = convert_to_row(img);
  cv::Mat to_out;
  const int pred_result = static_cast<int>(mlp->predict(to_pred, to_out));
  switch (pred_result) {
    case 0:
      return std::make_pair<char, int>('0', static_cast<int>(to_out.at<float>(0, pred_result) * 1000));
    case 1:
      return std::make_pair<char, int>('1', static_cast<int>(to_out.at<float>(0, pred_result) * 1000));
    case 2:
      return std::make_pair<char, int>('2', static_cast<int>(to_out.at<float>(0, pred_result) * 1000));
    case 3:
      return std::make_pair<char, int>('3', static_cast<int>(to_out.at<float>(0, pred_result) * 1000));
    case 4:
      return std::make_pair<char, int>('4', static_cast<int>(to_out.at<float>(0, pred_result) * 1000));
    case 5:
      return std::make_pair<char, int>('6', static_cast<int>(to_out.at<float>(0, pred_result) * 1000));
    case 6:
      return std::make_pair<char, int>('7', static_cast<int>(to_out.at<float>(0, pred_result) * 1000));
    case 7:
      return std::make_pair<char, int>('8', static_cast<int>(to_out.at<float>(0, pred_result) * 1000));
    case 8:
      return std::make_pair<char, int>('9', static_cast<int>(to_out.at<float>(0, pred_result) * 1000));
    case 9:
      return std::make_pair<char, int>('B', static_cast<int>(to_out.at<float>(0, pred_result) * 1000));
    case 10:
      return std::make_pair<char, int>('C', static_cast<int>(to_out.at<float>(0, pred_result) * 1000));
    case 11:
      return std::make_pair<char, int>('D', static_cast<int>(to_out.at<float>(0, pred_result) * 1000));
    case 12:
      return std::make_pair<char, int>('E', static_cast<int>(to_out.at<float>(0, pred_result) * 1000));
    case 13:
      return std::make_pair<char, int>('G', static_cast<int>(to_out.at<float>(0, pred_result) * 1000));
    case 14:
      return std::make_pair<char, int>('H', static_cast<int>(to_out.at<float>(0, pred_result) * 1000));
    case 15:
      return std::make_pair<char, int>('J', static_cast<int>(to_out.at<float>(0, pred_result) * 1000));
    case 16:
      return std::make_pair<char, int>('K', static_cast<int>(to_out.at<float>(0, pred_result) * 1000));
    case 17:
      return std::make_pair<char, int>('L', static_cast<int>(to_out.at<float>(0, pred_result) * 1000));
    case 18:
      return std::make_pair<char, int>('R', static_cast<int>(to_out.at<float>(0, pred_result) * 1000));
    case 19:
      return std::make_pair<char, int>('S', static_cast<int>(to_out.at<float>(0, pred_result) * 1000));
    case 20:
      return std::make_pair<char, int>('T', static_cast<int>(to_out.at<float>(0, pred_result) * 1000));
    default:
      return std::make_pair<char, int>('\0', 0);
  }
}

FoundNumber RecognizeNumberByGroup(cv::Mat& etal, const FigureGroup& group, const cv::Mat& original, number_data& stat_data) {
  FoundNumber result;

  /*for (const auto& next : group) {
    const cv::Mat copy = etal(cv::Rect(next.left(), next.top(), next.width() + 1, next.height() + 1)).clone();
    const std::pair<char, int> rec_res = RecognizeSymbol(copy);
    result.m_number += rec_res.first;
    result.m_weight += rec_res.second;
    result.m_figs.push_back(next);
  }

  assert(result.m_number.size() == group.size());*/

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

cv::Mat CreateGrayImage(const cv::Mat& input) {
  cv::Mat gray(input.size(), CV_8U);
  if (input.channels() == 1) {
    gray = input.clone();
  } else if (input.channels() == 3 || input.channels() == 4) {
    cvtColor(input, gray, CV_RGB2GRAY);
  } else {
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
                ret.push_back(fig_to_create);
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
  sort(ret.begin(), ret.end(), FigLessLeft);
  return ret;
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

ParseToFigsResult ParseToFigures(const cv::Mat& input, int level) {
  ParseToFigsResult result;
  result.level = level;
  cv::Mat gray_image = CreateGrayImage(input);
//  cv::Mat wolf_out = gray_image.clone();
//  NiblackSauvolaWolfJolion(gray_image, wolf_out, WOLFJOLION, next_level, next_level, k);
  cv::Mat wolf_out = gray_image > level;
  ReplaceBlackWhite(wolf_out); // конвертим для сингапура
  result.bin_image = wolf_out.clone();
  result.figs = ParseToFigures<0>(wolf_out);
  // выкидываем слишком узкие штуки
  RemoveTooNarrowFigures(result.figs);
  return result;
}

std::vector<ParseToFigsResult> ParseToFigures(const cv::Mat& input, const std::vector<int>& wins) {
  std::vector<ParseToFigsResult> result;
  for (int next_level : wins) {
    const ParseToFigsResult next_res = ParseToFigures(input, next_level);
    if (!next_res.figs.empty()) {
      result.push_back(next_res);
    }
  }
  return result;
}

/*std::vector<std::pair<float, float>> Singapore8SymsKoeffs() {
  std::vector<std::pair<float, float>> result;
  return result;
}

bool IsValid(std::array<int, 8> syms, const FigureGroup& figs, float avarage_width) {
  // проверяем что бы между соседними символами нельзя было всунуть еще один символ
  for (int nn = 0; nn < 8 - 1; ++nn) {
    if (syms[nn] != -1 && syms[nn + 1] != -1) {
      const double dist = cv::norm(figs[syms[nn]].CenterCV() - figs[syms[nn + 1]].CenterCV());
      if (dist > 1.5 * avarage_width) {
        return false;
      }
    }
  }
  // проверяем что бы между символами с пропусками можно было всунуть сколько нужно
  for (int nn = 0; nn < 8 - 1; ++nn) {
    if (syms[nn] != -1 && syms[nn + 1] == -1) {
      std::array<int, 8>::iterator next_valid_iter = find_if_not(begin(syms) + nn + 1, end(syms), bind2nd(std::equal_to<int>(), -1));
      if (next_valid_iter != end(syms)) {
        const double dist = cv::norm(figs[syms[nn]].CenterCV() - figs[*next_valid_iter].CenterCV());
        if (dist < avarage_width * (*next_valid_iter - syms[nn] + 1)) {
          return false;
        }
      }
    }
  }
  return true;
}

bool MoveNext(std::array<int, 8>& syms) {
  int to_move = -1;
  reverse(begin(syms), end(syms));
  {
    std::array<int, 8>::iterator first_invalid_iter = find_if(begin(syms), end(syms), bind2nd(std::equal_to<int>(), -1));
    if (first_invalid_iter == end(syms)) {
      reverse(begin(syms), end(syms));
      return false;
    }
    const size_t first_invalid_index = distance(begin(syms), first_invalid_iter);
    std::array<int, 8>::iterator first_valid_iter = find_if_not(first_invalid_iter, end(syms), bind2nd(std::equal_to<int>(), -1));
    if (first_valid_iter == end(syms)) {
      reverse(begin(syms), end(syms));
      return false;
    }
    const size_t first_valid_index = distance(begin(syms), first_valid_iter);
    syms[first_valid_index - 1] = syms[first_valid_index];
    syms[first_valid_index] = -1;
    to_move = 8 - (static_cast<int>(first_valid_index) - 1);
  }
  reverse(begin(syms), end(syms));
  // подтягиваем вверх что уже было сдвинуто
  while (to_move < 8 && to_move > 0) {
    if (syms[to_move] == -1) {
      std::array<int, 8>::iterator first_valid_iter = find_if_not(begin(syms) + to_move, end(syms), bind2nd(std::equal_to<int>(), -1));
      if (first_valid_iter == syms.end()) {
        break;
      } else {
        syms[to_move] = *first_valid_iter;
        *first_valid_iter = -1;
      }
    }
    ++to_move;
  } 
  return true;
}

std::vector<std::array<int, 8>> CollectCorredVariants(const FigureGroup& group) {
  const float avarage_height = static_cast<float>(accumulate(begin(group), end(group), 0, [] (int prev, const Figure& other) -> int { return other.height() + prev; })) / static_cast<float>(group.size());
  const float avarage_width = static_cast<float>(accumulate(begin(group), end(group), 0, [] (int prev, const Figure& other) -> int { return other.width() + prev; })) / static_cast<float>(group.size());
  std::vector<std::array<int, 8>> result;
  bool do_next = true;
  std::array<int, 8> next_order;
  for (size_t nn = 0; nn < std::max(group.size(), static_cast<size_t>(8)); ++nn) {
    next_order[nn] = (nn < group.size() ? static_cast<int>(nn) : -1);
  }
  while (do_next) {
    if (IsValid(next_order, group, avarage_width)) {
      result.push_back(next_order);
    }
    do_next = MoveNext(next_order);
  }

  return result;
}*/

bool IsIntersect(const Figure& fig, const FigureGroup& group) {
  for (const Figure& next_fig: group) {
    const cv::Rect2i& next_rc = next_fig.RectCV();
    const cv::Rect2i& fig_rc = fig.RectCV();
    if ((next_rc & fig_rc).area() > 0) {
      return true;
    }
  }
  return false;
}

bool IsNotTooFarFromGroup(const Figure& fig, const FigureGroup& group) {
  const double average_height = static_cast<double>(accumulate(begin(group), end(group), 0, [](int prev, const Figure& fig) -> int { return prev + fig.height(); })) / static_cast<double>(group.size());
  const int max_diff_x = static_cast<int>(average_height * static_cast<double>(kMaxDistByX));
  const int max_diff_y = static_cast<int>(average_height * static_cast<double>(kMaxDistByY));

  for (auto gr_iter = begin(group); gr_iter != end(group); ++gr_iter) {
    if (std::abs(gr_iter->CenterCV().x - fig.CenterCV().x) >= max_diff_x) {
      return false;
    }
    if (std::abs(gr_iter->CenterCV().y - fig.CenterCV().y) >= max_diff_y) {
      return false;
    }
  }
  return true;
}

std::vector<FigureGroup> ProcessFoundGroups(int curr_level, cv::Mat& input_mat, const std::vector<FigureGroup>& input_groups, const std::vector<ParseToFigsResult>& other_figs) {
  std::vector<int> levels = {curr_level + 10, curr_level - 10, curr_level + 20, curr_level - 20};
  std::remove_if(begin(levels), end(levels), [](int to_check) -> bool { return to_check <= 0 || to_check >= 255; });
  std::vector<FigureGroup> result;
  for (const FigureGroup& next_group : input_groups) {
    FigureGroup composed_group = next_group;
    for (const int lev_to_check : levels) {
      std::vector<ParseToFigsResult>::const_iterator it_fig_level = std::find_if(begin(other_figs), end(other_figs), [lev_to_check](const ParseToFigsResult& in)->bool { return in.level == lev_to_check; });
      if (it_fig_level != end(other_figs)) {
        for (const Figure& fig: it_fig_level->figs) {
          // check intercect
          if (!IsIntersect(fig, composed_group)) {
            // check angle
            if (IsFigFitToGroupAngle(fig, next_group, CalculateAngle(next_group))) {
              // check size
              if (IsFitByHeight(fig, next_group.at(0))) {
                // check distance
                if (IsNotTooFarFromGroup(fig, next_group)) {
                  // copy fig-mat to dest-mat
                  for (int nn = fig.left(); nn <= fig.right(); ++nn) {
                    for (int mm = fig.top(); mm <= fig.bottom(); ++mm) {
                      input_mat.at<uchar>(cv::Point2i(nn, mm)) = it_fig_level->bin_image.at<uchar>(cv::Point2i(nn, mm));
                    }
                  }
                  // search pos and insert
                  auto pos_iter = find_if(begin(composed_group), end(composed_group), [fig](const Figure& check_fig) -> bool { return fig.left() < check_fig.left(); });
                  composed_group.insert(pos_iter, fig);
                }
              }
            }
          }
        }
      }
    }
    result.push_back(composed_group);

/*    std::vector<std::array<int, 8>> all_groups = CollectCorredVariants(next_group);
    // пытаемся найти фигуры после
    for (std::array<int, 8> next_array_candidate : all_groups) {
    }

    std::stringstream ss;
    for (size_t nn = 0; nn < all_syms.size(); ++nn) {
      for (int mm = 0; mm < 8; ++mm) {
        ss << all_syms[nn][mm];
      }
      ss << std::endl;
    }*/
  }

  return result;
}

std::vector<FigureGroup> MakeGroups(const FigureGroup& figs) {
  FigureGroup figs_clone(figs);
  std::vector<FigureGroup> groups = MakeGroupsByAngle(figs_clone);

  /*  for (int nn = 0; nn < groups.size(); ++nn) {
  cv::Mat ccc = result.first.clone();
  cv::Mat kkk;
  cvtColor(ccc, kkk, CV_GRAY2RGB);
  std::vector<FigureGroup> cur;
  cur.push_back(groups.at(nn));
  DrawGroupsFigures(kkk, cur, CV_RGB(255, 0, 0));
  }*/
  // удаляем лишнее по ширине
  RemoteTooFarFromFirst(groups, kMaxDistByX, std::bind(&Figure::left, std::placeholders::_1));
  // удаляем лишнее по длине
  RemoteTooFarFromFirst(groups, kMaxDistByY, std::bind(&Figure::bottom, std::placeholders::_1));
  // выкидываем элементы, не пропорциональные первому элементу
  RemoveNotFitToFirstByHeight(groups);
  // выкидываем группы, которые включаются в другие группы
  RemoveIncludedGroups(groups);
  // сливаем пересекающиеся группы
  MergeIntersectsGroups(groups);

  return groups;
}

std::pair<cv::Mat, std::vector<FigureGroup>> ParseToGroups(const cv::Mat& input, int level) {
  std::pair<cv::Mat, std::vector<FigureGroup>> result;
  const ParseToFigsResult par_fig_res = ParseToFigures(input, level);
  result.first = par_fig_res.bin_image;
  result.second = MakeGroups(par_fig_res.figs);

  return result;
}

template<int stat_data_index>
std::vector<FoundNumber> ReadNumberImpl(const cv::Mat& input, int gray_level, number_data& stat_data) {
  return std::vector<FoundNumber>();
//  const cv::Mat& gray_image = CreateGrayImage(input);
//  cv::Mat img_bw = gray_image > gray_level;
//
//  cv::Mat iii1 = img_bw.clone();
//  NiblackSauvolaWolfJolion(gray_image, iii1, WOLFJOLION, gray_level / 10, gray_level / 10, 0.05);
////  cv::Mat iii2 = img_bw.clone();
////  NiblackSauvolaWolfJolion(gray_image, iii2, WOLFJOLION, 7, 7, 0.1);
////  cv::Mat iii3 = img_bw.clone();
////  NiblackSauvolaWolfJolion(gray_image, iii3, WOLFJOLION, 7, 7, 0.15);
//
//
//  //ReplaceBlackWhite(img_bw); // конвертим для сингапура
//  //cv::Mat img_to_recog = img_bw.clone();
//  //FigureGroup figs = ParseToFigures<stat_data_index>(img_bw);
//
//  ReplaceBlackWhite(iii1); // конвертим для сингапура
//  cv::Mat img_to_recog = iii1.clone();
//  FigureGroup figs = ParseToFigures<stat_data_index>(iii1);
//
////  cv::Mat mama = input.clone();
////  DrawFigures(mama, figs);
//
//  // выкидываем слишком узкие штуки
//  RemoveTooNarrowFigures(figs);
//  std::vector<FigureGroup> groups = MakeGroupsByAngle(figs);
//  // удаляем лишнее по ширине
//  RemoteTooFarFromFirst(groups, 7, std::bind(&Figure::left, std::placeholders::_1));
//  // удаляем лишнее по длине
//  RemoteTooFarFromFirst(groups, 3, std::bind(&Figure::left, std::placeholders::_1));
//  // выкидываем элементы, не пропорциональные первому элементу
//  RemoveNotFitToFirstByHeight(groups);
//  // выкидываем группы, которые включаются в другие группы
//  RemoveIncludedGroups(groups);
//  // сливаем пересекающиеся группы
//  MergeIntersectsGroups(groups);
//  cv::Mat deb = input.clone();
//  DrawGroupsFigures(deb, groups, CV_RGB(255, 0, 0));
//  // ищем номера
//  const std::vector<FoundNumber> nums = RecognizeNumber(img_to_recog, groups, gray_image, stat_data);
//  return nums;
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
			const cv::Mat& gray_image = CreateGrayImage( input );
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
    (void)tt;
    assert(false);
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

std::vector<ParseToGroupWithProcessingResult> ParseToGroupWithProcessing(const cv::Mat& input) {
  std::vector<ParseToGroupWithProcessingResult> result;

  // находим все фигуры
  std::vector<int> steps = {140, 150};
  //std::vector<int> steps;
  //const int kStepSize = 10;
  //for (int nn = 0; nn < 255;  nn += kStepSize) {
  //  steps.push_back(nn);
  //}

  std::vector<ParseToFigsResult> all_figs;
  for (int nn : steps) {
    all_figs.push_back(ParseToFigures(input, nn));
  }

  // составляем группы с учетом символов с других уровней
  for (int nn : steps) {
    auto cur_lev_figs = std::find_if(begin(all_figs), end(all_figs), [nn](const ParseToFigsResult& other) -> bool { return other.level == nn; });
    assert(cur_lev_figs != end(all_figs));
    const std::vector<FigureGroup> parsed_groups = MakeGroups(cur_lev_figs->figs);
    const std::vector<FigureGroup> process_groups = ProcessFoundGroups(nn, cur_lev_figs->bin_image, parsed_groups, all_figs);
    for (size_t mm = 0; mm < process_groups.size(); ++mm) {
      ParseToGroupWithProcessingResult next;
      next.img = cur_lev_figs->bin_image.clone();
      next.level = nn;
      next.group = process_groups.at(mm);
      if (next.group.size()) {
        result.push_back(next);
      }
    }
  }

  // распоздаем символы в группах
  for (ParseToGroupWithProcessingResult& next_group: result) {
    for (const Figure& fig : next_group.group) {
      std::pair<char, int> next_sym = RecognizeSymbol(next_group.img(fig.RectCV()).clone());
      next_group.number += (next_sym.first == '\0' ? '?' : next_sym.first);
      next_group.weights.push_back(next_sym.second);
    }
  }

  return result;
}