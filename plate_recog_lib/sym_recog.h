#pragma once
#include <string>
#include <set>

namespace cv
{
	class Mat;
}
class figure;

const int data_width = 15;
const int data_height = 22;

int search_max_val( const cv::Mat& data, int row = 0 );
// convert to necessary format
cv::Mat convert_to_row( const cv::Mat& input );
// read from file and convert to necessary format
cv::Mat from_file_to_row( const std::string& file_name );
// recognize char
std::pair< char, double > proc_char( const cv::Mat& input );
// recognize number
std::pair< char, double > proc_num( const cv::Mat& input );
// init recognizer
void init_recognizer();
// region codes
const std::set< std::string >& region_codes();
