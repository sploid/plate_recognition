template< int stat_data_index >
void search_region(FoundNumber& best_number, const int best_level, const cv::Mat& original, number_data& stat_data) {
  const FigureGroup& figs = best_number.m_figs;
  if (figs.size() != 6) // ищем регион только если у нас есть все 6 символов
  {
    return;
  }

  const double koef_height = 0.76; // отношение высоты буквы к высоте цифры
                                   // средняя высота буквы
  const double avarage_height = (static_cast< double >(figs.at(0).height() + figs.at(4).height() + figs.at(5).height()) / 3.
                                 + static_cast< double >(figs.at(1).height() + figs.at(2).height() + figs.at(3).height()) * koef_height / 3.) / 2.;

  // ищем угол наклона по высоте (считаем среднее между не соседними фигурами 0-4, 0-5, 1-3)
  std::vector< pair_int > index_fig_to_angle;
  index_fig_to_angle.push_back(std::make_pair(0, 4));
  index_fig_to_angle.push_back(std::make_pair(0, 5));
  index_fig_to_angle.push_back(std::make_pair(1, 3));
  double avarage_angle_by_y = 0.; // средний угол наклона номера по оси У
  int move_by_y = 0; // номер наклонен вверх или вниз
  for (size_t nn = 0; nn < index_fig_to_angle.size(); ++nn) {
    const pair_int& cur_in = index_fig_to_angle.at(nn);
    avarage_angle_by_y += atan(static_cast< double >(figs[cur_in.first].center().second - figs[cur_in.second].center().second) / static_cast< double >(figs[cur_in.first].center().first - figs[cur_in.second].center().first));
    move_by_y += figs[cur_in.first].center().second - figs[cur_in.second].center().second;
  }
  avarage_angle_by_y = avarage_angle_by_y / static_cast< double >(index_fig_to_angle.size());
  const double sin_avarage_angle_by_y = sin(avarage_angle_by_y);

  std::vector< int > levels_to_iterate;
  for (int nn = best_level - 20; nn <= best_level + 20; nn += 10) {
    levels_to_iterate.push_back(nn);
  }
  levels_to_iterate.erase(remove_if(levels_to_iterate.begin(), levels_to_iterate.end(), &not_in_char_distance), levels_to_iterate.end());
  //	levels_to_iterate.push_back( best_level + 10 );

  std::vector< FoundNumber > nums;
  for (size_t nn = 0; nn < levels_to_iterate.size(); ++nn) {
    cv::Mat etal = original > levels_to_iterate.at(nn);
    {
      FoundNumber fs2;
      const std::vector< std::vector< pair_doub > > move_reg_by_2_sym_reg(get_2_sym_reg_koef(figs, avarage_height, sin_avarage_angle_by_y));
      search_region_symbol< stat_data_index >(fs2, etal, original, calc_center(figs, move_reg_by_2_sym_reg, 0), avarage_height, false, stat_data);
      const pair_int next_2 = get_pos_next_in_region(figs, move_reg_by_2_sym_reg, 1, fs2, avarage_height);
      search_region_symbol< stat_data_index >(fs2, etal, original, next_2, avarage_height, true, stat_data);
      nums.push_back(fs2);
    }
    {
      FoundNumber fs3;
      const std::vector< std::vector< pair_doub > > move_reg_by_3_sym_reg(get_3_sym_reg_koef(figs, avarage_height, sin_avarage_angle_by_y));
      search_region_symbol< stat_data_index >(fs3, etal, original, calc_center(figs, move_reg_by_3_sym_reg, 0), avarage_height, false, stat_data);
      const pair_int next_2 = get_pos_next_in_region(figs, move_reg_by_3_sym_reg, 1, fs3, avarage_height);
      /*			pair_int next_2 = calc_center( figs, move_reg_by_3_sym_reg, 1 );
      if ( fs3.m_number.at( fs3.m_number.size() - 1 ) != '?' )
      {
      next_2 = fs3.m_figs.at( fs3.m_figs.size() - 1 ).center() + std::make_pair( static_cast< int >( 0.75 * avarage_height ), 0 );
      }*/
      search_region_symbol< stat_data_index >(fs3, etal, original, next_2, avarage_height, false, stat_data);
      const pair_int next_3 = get_pos_next_in_region(figs, move_reg_by_3_sym_reg, 2, fs3, avarage_height);
      /*			pair_int next_3 = calc_center( figs, move_reg_by_3_sym_reg, 2 );
      if ( fs3.m_number.at( fs3.m_number.size() - 1 ) != '?' )
      {
      next_3 = fs3.m_figs.at( fs3.m_figs.size() - 1 ).center() + std::make_pair( static_cast< int >( 0.75 * avarage_height ), 0 );
      }*/
      search_region_symbol< stat_data_index >(fs3, etal, original, next_3, avarage_height, true, stat_data);
      nums.push_back(fs3);
    }
  }
  sort(nums.begin(), nums.end(), &compare_regions);
  std::vector< FoundNumber >::const_iterator it = max_element(nums.begin(), nums.end(), &compare_regions);

  best_number.m_number.append(it->m_number);
  best_number.m_weight += it->m_weight;
  for (size_t nn = 0; nn < it->m_figs.size(); ++nn) {
    best_number.m_figs.push_back(it->m_figs.at(nn));
  }

}

pair_int get_pos_next_in_region(const FigureGroup& figs, const std::vector< std::vector< pair_doub > >& move_reg, const int index, const FoundNumber& number, const double avarage_height) {
  pair_int ret = calc_center(figs, move_reg, index);
  if (number.m_number.at(number.m_number.size() - 1) != '?') {
    return number.m_figs.at(number.m_figs.size() - 1).center() + std::make_pair(static_cast< int >(0.75 * avarage_height), 0);
  }
  return ret;
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

bool not_in_char_distance(int val) {
  return val <= 0 || val >= 255;
}

// выбираем что лучше подходит 2-х или 3-х символьный регион
std::vector< sym_info > select_sym_info(const std::vector< sym_info >& fs2, const std::vector< sym_info >& fs3) {
  assert(fs2.size() == 2U);
  assert(fs3.size() == 3U);
  if (fs3.at(0).m_symbol != '1' && fs3.at(0).m_symbol != '7') {
    return fs2;
  } else if (fs3.at(0).m_symbol == '?' || fs3.at(1).m_symbol == '?' || fs3.at(2).m_symbol == '?') {
    return fs2;
  } else {
    return fs3;
  }
}

std::vector< std::vector< pair_doub > > get_2_sym_reg_koef(const FigureGroup& figs, double avarage_height, double sin_avarage_angle_by_y) {
  std::vector< std::vector< pair_doub > > ret;
  fill_reg(ret, 6.7656, -0.4219, 7.5363, -0.3998);
  fill_reg(ret, 5.6061, -0.2560, 6.3767, -0.2338);
  fill_reg(ret, 4.6016, -0.2991, 5.3721, -0.2769);
  fill_reg(ret, 3.5733, -0.3102, 4.3440, -0.2880);
  fill_reg(ret, 2.4332, -0.4802, 3.2039, -0.4580);
  fill_reg(ret, 1.4064, -0.5123, 2.1770, -0.4901);
  apply_angle(figs, ret, avarage_height, sin_avarage_angle_by_y);
  return ret;
}

std::vector< std::vector< pair_doub > > get_3_sym_reg_koef(const FigureGroup& figs, double avarage_height, double sin_avarage_angle_by_y) {
  std::vector< std::vector< pair_doub > > ret;
  fill_reg(ret, 5.8903, -0.3631, 6.6165, -0.3631, 7.3426, -0.3631);
  fill_reg(ret, 4.9220, -0.2017, 5.6482, -0.2017, 6.3744, -0.2017);
  fill_reg(ret, 3.9537, -0.2421, 4.6799, -0.2421, 5.4061, -0.2421);
  fill_reg(ret, 2.9855, -0.2421, 3.7117, -0.2421, 4.4379, -0.2421);
  fill_reg(ret, 2.0172, -0.4034, 2.7434, -0.4034, 3.4696, -0.4034);
  fill_reg(ret, 1.0490, -0.4034, 1.7752, -0.4034, 2.5013, -0.4034);
  apply_angle(figs, ret, avarage_height, sin_avarage_angle_by_y);
  return ret;
}

template< int stat_data_index >
void search_region_symbol(FoundNumber& number, const cv::Mat& etal, const cv::Mat& origin, const pair_int& reg_center, const double avarage_height, bool last_symbol, number_data& stat_data) {
  //	number.m_figs.push_back( Figure( reg_center, std::make_pair( 1, 1 ) ) );
  const pair_int nearest_black = search_nearest_black(etal, reg_center);
  //	number.m_figs.push_back( Figure( nearest_black, std::make_pair( 1, 1 ) ) );
  //	return;

  if (nearest_black.first != -1
      && nearest_black.second != -1) {
    Figure top_border_fig;
    Figure conture_fig;
    cv::Mat to_search = etal.clone();
    // Ищем контуры по верхней границе
    add_pixel_as_spy< stat_data_index >(nearest_black.second, nearest_black.first, to_search, top_border_fig, -1, nearest_black.second + 1);
    if (top_border_fig.top() > reg_center.second - static_cast< int >(avarage_height)) // ушли не далее чем на одну фигуру
    {
      cv::Mat to_contur = etal.clone();
      add_pixel_as_spy< stat_data_index >(nearest_black.second, nearest_black.first, to_contur, conture_fig, -1, top_border_fig.top() + static_cast< int >(avarage_height) + 1);
      if (conture_fig.width() >= static_cast< int >(avarage_height)) // если широкая фигура, то скорее всего захватили рамку
      {
        // центрируем фигуру по Х (вырезаем не большой кусок и определяем его центр)
        cv::Mat to_stable = etal.clone();
        Figure short_fig;
        // если центр сильно уехал, этот фокус не сработает, т.к. мы опять захватим рамку
        add_pixel_as_spy< stat_data_index >(nearest_black.second, nearest_black.first, to_stable, short_fig, nearest_black.second - static_cast< int >(avarage_height * 0.4), nearest_black.second + static_cast< int >(avarage_height * 0.4));
        conture_fig = Figure(pair_int(short_fig.center().first, reg_center.second), pair_int(static_cast< int >(avarage_height * 0.60), static_cast< int >(avarage_height)));
      } else if (last_symbol && conture_fig.width() >= static_cast< int >(avarage_height * 0.80)) // если последняя фигура, то могли захватить болтик черный
      {
        // Тут делаю обработку захвата болтика
        conture_fig = Figure(conture_fig.left(), conture_fig.left() + static_cast< int >(avarage_height * 0.60), conture_fig.top(), conture_fig.bottom());
      }
    } else {
      // центрируем фигуру по Х (вырезаем не большой кусок и определяем его центр)
      cv::Mat to_stable = etal.clone();
      Figure short_fig;
      // если центр сильно уехал, этот фокус не сработает, т.к. мы опять захватим рамку
      add_pixel_as_spy< stat_data_index >(nearest_black.second, nearest_black.first, to_stable, short_fig, nearest_black.second - static_cast< int >(avarage_height * 0.4), nearest_black.second + static_cast< int >(avarage_height * 0.4));
      if (!short_fig.is_empty()) {
        conture_fig = Figure(pair_int(short_fig.center().first, reg_center.second), pair_int(static_cast< int >(avarage_height * 0.60), static_cast< int >(avarage_height)));
      } else {
        conture_fig = Figure();
      }
    }
    if (!conture_fig.is_empty()
        && find(number.m_figs.begin(), number.m_figs.end(), conture_fig) == number.m_figs.end()
        && static_cast< double >(conture_fig.height()) > 0.5 * avarage_height
        && static_cast< double >(conture_fig.width()) > 0.2 * avarage_height
        ) {
      const std::pair< char, double > sym_sym = find_sym_nn(true, conture_fig, origin, stat_data);
      number.m_figs.push_back(conture_fig);
      number.m_number += sym_sym.first;
      number.m_weight += static_cast< int >(sym_sym.second);
    } else {
      number.m_number += '?';
    }
  } else {
    number.m_number += '?';
  }
}

// ищем ближайшую черную точку
pair_int search_nearest_black(const cv::Mat& etal, const pair_int& center) {
  pair_int dist_x(center.first, center.first);
  pair_int dist_y(center.second, center.second);
  for (;;) {
    for (int nn = dist_x.first; nn <= dist_x.second; ++nn) {
      for (int mm = dist_y.first; mm <= dist_y.second; ++mm) {
        const unsigned char cc = etal.at< unsigned char >(mm, nn);
        if (cc != 255) {
          assert(cc == 0);
          return pair_int(nn, mm);
        }
      }
    }
    --dist_x.first;
    ++dist_x.second;
    --dist_y.first;
    ++dist_y.second;
    if (dist_x.first < 0 || dist_x.second >= etal.cols
        || dist_y.first < 0 || dist_y.second >= etal.rows) {
      return pair_int(-1, -1);
    }
  }
}

void fill_reg(std::vector< std::vector< pair_doub > >& data, double x1, double y1, double x2, double y2, double x3, double y3) {
  std::vector< pair_doub > tt;
  tt.push_back(std::make_pair(x1, y1));
  tt.push_back(std::make_pair(x2, y2));
  tt.push_back(std::make_pair(x3, y3));
  data.push_back(tt);
}

void fill_reg(std::vector< std::vector< pair_doub > >& data, double x1, double y1, double x2, double y2) {
  std::vector< pair_doub > tt;
  tt.push_back(std::make_pair(x1, y1));
  tt.push_back(std::make_pair(x2, y2));
  data.push_back(tt);
}

