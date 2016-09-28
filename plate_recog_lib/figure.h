#pragma once
#include <vector>
#include <opencv2/opencv.hpp>

class Figure {
public:
  Figure()
    : m_left(-1),
      m_right(-1),
      m_top(-1),
      m_bottom(-1),
      m_too_big(false) {
  }

  Figure(const std::pair<int, int>& center, const std::pair<int, int>& size)
    : m_left(center.first - size.first / 2 - 1),
      m_right(center.first + size.first / 2 + 1),
      m_top(center.second - size.second / 2 - 1),
      m_bottom(center.second + size.second / 2 + 1),
      m_too_big(false) {
  }

  Figure(int left, int right, int top, int bottom)
    : m_left(left),
      m_right(right),
      m_top(top),
      m_bottom(bottom) {
  }

  int width() const {
    return m_right - m_left;
  }

  int height() const {
    return m_bottom - m_top;
  }

  bool is_empty() const {
    return m_left == -1 && m_right == -1 && m_top == -1 && m_bottom == -1;
  }

  void add_point(const std::pair<int, int>& val) {
    if (too_big()) {
      return;
    }

    if (m_left == - 1 || m_left > val.second) {
      m_left = val.second;
    }

    if (m_top == - 1 || m_top > val.first) {
      m_top = val.first;
    }

    if (m_right == - 1 || m_right < val.second) {
      m_right = val.second;
    }
		
    if (m_bottom == - 1 || m_bottom < val.first) {
      m_bottom = val.first;
    }

    if (m_right - m_left > 50) {
      m_too_big = true;
    }
  }

  cv::Point2i CenterCV() const {
    const int hor = m_left + (m_right - m_left) / 2;
    const int ver = m_top + (m_bottom - m_top) / 2;
    return cv::Point2i(hor, ver);
  }

  std::pair<int, int> center() const {
    const int hor = m_left + (m_right - m_left) / 2;
    const int ver = m_top + (m_bottom - m_top) / 2;
    return std::make_pair(hor, ver);
  }

  std::pair<int, int> top_left() const {
    return std::make_pair( left(), top() );
  }

  int left() const {
    return m_left;
  }

  int top() const {
    return m_top;
  }

  int right() const {
    return m_right;
  }

  int bottom() const {
    return m_bottom;
  }

  std::pair<int, int> bottom_right() const {
    return std::make_pair( right(), bottom() );
  }

  bool is_too_big() const {
    return !m_too_big && !is_empty();
  }

  bool too_big() const {
    return m_too_big;
  }

  bool operator<(const Figure& other) const {
    if (m_left != other.m_left)           return m_left < other.m_left;
    else if (m_top != other.m_top)        return m_top < other.m_top;
    else if (m_right != other.m_right)    return m_right < other.m_right;
    else if (m_bottom != other.m_bottom)  return m_bottom < other.m_bottom;
    else                                  return false;
  }

  bool operator==(const Figure& other) const {
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
typedef std::vector<Figure> FigureGroup;
