#pragma once
#include <opencv2/opencv.hpp>

std::string read_number( const cv::Mat& image );
std::vector< std::pair< std::string, int > > read_number( const cv::Mat& image, int grey_level );
