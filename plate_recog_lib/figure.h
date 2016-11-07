#pragma once
#include <vector>
#include <opencv2/opencv.hpp>

class Figure {
public:
  Figure()
    : left_(-1),
      right_(-1),
      top_(-1),
      bottom_(-1),
      m_too_big(false) {
  }

  Figure(const std::pair<int, int>& center, const std::pair<int, int>& size)
    : left_(center.first - size.first / 2 - 1),
      right_(center.first + size.first / 2 + 1),
      top_(center.second - size.second / 2 - 1),
      bottom_(center.second + size.second / 2 + 1),
      m_too_big(false) {
  }

  Figure(int left, int right, int top, int bottom)
    : left_(left),
      right_(right),
      top_(top),
      bottom_(bottom) {
  }

  double AngleTo(const Figure& other) const {
    return 57.2957795 * atan2(static_cast<double>(CenterCV().x - other.CenterCV().x), static_cast<double>(CenterCV().y - other.CenterCV().y));
  }

  int width() const {
    assert(!is_empty() && left_ <= right_);
    return right_ - left_ + 1;
  }

  int height() const {
    assert(!is_empty() && bottom_ >= top_);
    return bottom_ - top_ + 1;
  }

  bool is_empty() const {
    return left_ == -1 && right_ == -1 && top_ == -1 && bottom_ == -1;
  }

  void add_point(const std::pair<int, int>& val) {
    if (too_big()) {
      return;
    }

    if (left_ == - 1 || left_ > val.second) {
      left_ = val.second;
    }

    if (top_ == - 1 || top_ > val.first) {
      top_ = val.first;
    }

    if (right_ == - 1 || right_ < val.second) {
      right_ = val.second;
    }
		
    if (bottom_ == - 1 || bottom_ < val.first) {
      bottom_ = val.first;
    }

/*    if (right_ - left_ > 50) {
      m_too_big = true;
    }*/
  }

  cv::Point2i CenterCV() const {
    const int hor = left_ + (right_ - left_) / 2;
    const int ver = top_ + (bottom_ - top_) / 2;
    return cv::Point2i(hor, ver);
  }

  cv::Rect2i RectCV() const {
    return cv::Rect2i(cv::Point2i(left_, top_), cv::Point2i(right_ + 1, bottom_ + 1));
  }

  std::pair<int, int> center() const {
    const int hor = left_ + (right_ - left_) / 2;
    const int ver = top_ + (bottom_ - top_) / 2;
    return std::make_pair(hor, ver);
  }

  std::pair<int, int> top_left() const {
    return std::make_pair(left(), top());
  }

  int left() const {
    return left_;
  }

  int top() const {
    return top_;
  }

  int right() const {
    return right_;
  }

  int bottom() const {
    return bottom_;
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
    if (left_ != other.left_)             return left_ < other.left_;
    else if (top_ != other.top_)        return top_ < other.top_;
    else if (right_ != other.right_)      return right_ < other.right_;
    else if (bottom_ != other.bottom_)  return bottom_ < other.bottom_;
    else                                  return false;
  }

  bool operator==(const Figure& other) const {
    return left_ == other.left_
      && right_ == other.right_
      && top_ == other.top_
      && bottom_ == other.bottom_;
  }

private:
  int left_;
  int right_;
  int top_;
  int bottom_;
  bool m_too_big;
};

//typedef std::map< std::pair< bool, figure >, std::pair< char, double > > clacs_figs_type;
typedef std::vector<Figure> FigureGroup;
