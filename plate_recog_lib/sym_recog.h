#pragma once

#include <opencv2/opencv.hpp>

const int data_width = 15;
const int data_height = 22;

inline void proc( const cv::Mat& input )
{
	using namespace cv;
	using namespace std;

	static CvANN_MLP* mlp = 0;
	// todo: переделать на чтение из файла
	if ( mlp == 0 )
	{
		try
		{
		}
		catch ( const exception& e )
		{
			string ss( e.what() );
			int t = 0;
		}
	}

}