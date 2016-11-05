#pragma once
#include <iterator>
#include <numeric>
#include <string>
#include <vector>
#include <utility>
#include "figure.h"

namespace cv {
  class Mat;
}

// @todo: сделать документацию
// @todo: !!!!!!!!!!!!!если номер будет наклонным, то ширина будет не пропорциональная высоте (надо вводить косинусь угла наклона)!!!!!!!!!!!!!!!!!!!
// @todo: переделать мепинг символов к индексам массива, свитч лажа
// @todo: при распознавании символа, растянуть всю область, что бы убрать белый бордер, если он есть
// @todo: сделать перебор g_points_dublicate_first итераторами, а не индексами
// @todo: внести в настройки угол наклона номера

// @todo: M520PX190.jpg - большой угол и слева шуруп, поэтому левая буква М не распознается

// найденный номер
struct FoundNumber {
  std::string m_number;		// номер
  int m_weight = -1;		// вес
  FigureGroup m_figs;

  std::pair<std::string, int> to_pair() const {
    return std::make_pair(m_number, m_weight);
  }

  bool operator<(const FoundNumber& other) const {
    const int cnp = CountNotParsedSyms();
    const int cnp_other = other.CountNotParsedSyms();
    if (m_number.size() == other.m_number.size() && cnp != cnp_other) {
      return cnp > cnp_other;
    } else {
      return m_weight < other.m_weight;
    }
  }

  bool is_valid() const {
    return m_weight != -1 && !m_number.empty();
  }

  int CountNotParsedSyms() const {
    if (m_number.empty()) {
      return kCountNotParsedSyms; // вообще ничего нет
    }
    return static_cast<int>(std::count(m_number.begin(), m_number.end(), kUnknownSym));
  }

  static const int kCountNotParsedSyms = 100;
  static const char kUnknownSym = '?';
};


FoundNumber read_number(const cv::Mat& image, int gray_step = 0);
FoundNumber read_number_by_level(const cv::Mat& image, int gray_level);
struct ParseToFigsResult {
  cv::Mat bin_image;
  int level;
  FigureGroup figs;
};
ParseToFigsResult ParseToFigures(const cv::Mat& input, int level);
std::vector<ParseToFigsResult> ParseToFigures(const cv::Mat& input, const std::vector<int>& levels);
std::pair<cv::Mat, std::vector<FigureGroup>> ParseToGroups(const cv::Mat& input, int level);
std::vector<FigureGroup> ProcessFoundGroups(int curr_level, cv::Mat& input_mat, const std::vector<FigureGroup>& input_groups, const std::vector<ParseToFigsResult>& other_figs);

template<class T>
void CopyAndRemove(T& from, T& to, int index) {
  copy(begin(from) + index, end(from), back_inserter(to));
  from.erase(begin(from) + index, end(from));
}

struct ParseToGroupWithProcessingResult {
  cv::Mat img;
  int level = 0;
  FigureGroup group;
  std::string number;

  int Weight() const {
    return std::accumulate(begin(weights_), end(weights_), 0);
  }

  bool operator<(const ParseToGroupWithProcessingResult& other) const {
    return Weight() < other.Weight();
  }

  void AddSymbol(char sym, int weight) {
    number += sym;
    weights_.push_back(weight);
  }

  ParseToGroupWithProcessingResult Cut(int from) {
    ParseToGroupWithProcessingResult result;
    result.img = img;
    result.level = level;
    CopyAndRemove(group, result.group, from);
    CopyAndRemove(number, result.number, from);
    CopyAndRemove(weights_, result.weights_, from);
    return result;
  }

private:
  std::vector<int> weights_;
};

std::vector<ParseToGroupWithProcessingResult> ParseToGroupWithProcessing(const cv::Mat& input);

struct InitData {
  std::string mlp_train_path;
  double symbol_hei_to_wid_max_koef;
  double sym_to_sym_max_angle;
  int min_height;
};
void InitEngine(const InitData& data);

