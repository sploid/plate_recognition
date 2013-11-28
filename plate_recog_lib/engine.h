#pragma once
#include <string>
#include <utility>
#include "plate_recog_lib_config.h"

namespace cv
{
	class Mat;
}

struct recog_debug_callback
{
	virtual void out_image( const cv::Mat& image ) = 0;
	virtual void out_string( const std::string& text ) = 0;
};

// @todo: сделать документацию
// @todo: сделать как-нибудь отличия буквы О от буквы С. Можно поправить символы в файле syms
// @todo: увеличить скорость разбития картинки на фигуры
// @todo: реализовать обработку наклона номера
// todo: !!!!!!!!!!!!!если номер будет наклонным, то ширина будет не пропорциональная высоте (надо вводить косинусь угла наклона)!!!!!!!!!!!!!!!!!!!

std::pair< std::string, int > PLATE_RECOG_EXPORT read_number( const cv::Mat& image, recog_debug_callback *recog_debug );
std::pair< std::string, int > PLATE_RECOG_EXPORT read_number_by_level( const cv::Mat& image, int gray_level, recog_debug_callback *recog_debug );
