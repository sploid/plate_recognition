#pragma once
#include <string>
#include <vector>
#include <utility>
#include "plate_recog_lib_config.h"

namespace cv
{
	class Mat;
}

std::string PLATE_RECOG_EXPORT read_number( const cv::Mat& image );
