#pragma once
#include "plate_recog_lib_config.h"
#include <string>

namespace cv
{
	class Mat;
}
class figure;

const int data_width = 15;
const int data_height = 22;

int PLATE_RECOG_EXPORT search_max_val( const cv::Mat& data, int row = 0 );
// convert to necessary format
cv::Mat PLATE_RECOG_EXPORT convert_to_row( const cv::Mat& input );
// read from file and convert to necessary format
cv::Mat PLATE_RECOG_EXPORT from_file_to_row( const std::string& file_name );
// recognize char
std::pair< char, double > PLATE_RECOG_EXPORT proc_char( const cv::Mat& input );
// recognize number
std::pair< char, double > PLATE_RECOG_EXPORT proc_num( const cv::Mat& input );
// read configuration of neural networks
void PLATE_RECOG_EXPORT read_nn_config( const std::string& num_file_name, const std::string& char_file_name );