#pragma once
#include <string>
#include <set>
#include <vector>

namespace cv {
  class Mat;
}
class Figure;

const int kDataWidth = 5;
const int kDataHeight = 10;

int search_max_val(const cv::Mat& data, int row = 0);
// convert to necessary format
cv::Mat convert_to_row(const cv::Mat& input);
// read from file and convert to necessary format
cv::Mat from_file_to_row(const std::string& file_name);
// recognize char
std::pair<char, double> proc_char(const cv::Mat& input);
// recognize number
std::pair<char, double> proc_num(const cv::Mat& input);
// init recognizer
void init_recognizer();
// region codes
//const std::set< std::string >& region_codes();
// set folder for output symbols
void set_output_symbol_folder(const std::string& folder);
