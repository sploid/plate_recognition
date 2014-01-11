#pragma once
#include <string>
#include <utility>
#include "plate_recog_lib_config.h"

namespace cv
{
	class Mat;
}

// @todo: сделать документацию
// @todo: увеличить скорость разбития картинки на фигуры
// @todo: отрефакторить что бы не висло на картинке C967PO190.jpg
// @todo: !!!!!!!!!!!!!если номер будет наклонным, то ширина будет не пропорциональная высоте (надо вводить косинусь угла наклона)!!!!!!!!!!!!!!!!!!!
// @todo: переделать мепинг символов к индексам массива, свитч лажа
// @todo: при распознавании символа, растянуть всю область, что бы убрать белый бордер, если он есть
// @todo: частично автоматизировать обучение сети
// @todo: сделать перебор g_points_dublicate_first итераторами, а не индексами

// @todo: M520PX190.jpg - большой угол и слева шуруп, поэтому левая буква М не распознается

// если gray_step == -1, то использует некоторый алгоритм поиска оптимльной яркости, в ином случае просто идет по шагам
std::pair< std::string, int > PLATE_RECOG_EXPORT read_number( const cv::Mat& image, int gray_step = 0 );
std::pair< std::string, int > PLATE_RECOG_EXPORT read_number_by_level( const cv::Mat& image, int gray_level );
