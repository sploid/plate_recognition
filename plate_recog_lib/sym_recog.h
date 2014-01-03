#pragma once

#include <opencv2/opencv.hpp>

const int data_width = 15;
const int data_height = 22;

inline int search_max_val( const cv::Mat& data, int row = 0 )
{
	int max_col = 0;
	float max_val = data.at< float >( row, 0 );
	for ( int nn = 1; nn < data.cols; ++nn )
	{
		if ( data.at< float >( row, nn ) > max_val )
		{
			max_col = nn;
			max_val = data.at< float >( row, nn );
		}
	}
	return max_col;
}

// convert to necessary format
inline cv::Mat convert_to_row( const cv::Mat& input )
{
	using namespace std;
	using namespace cv;

	Mat one_chan_gray;
	if ( input.channels() == 3 && input.depth() == CV_MAT_DEPTH( CV_8U ) )
	{
		Mat gray( input.size(), CV_8U );
		cvtColor( input, one_chan_gray, CV_RGB2GRAY );
	}
	else if ( input.channels() == 1 && input.depth() == CV_MAT_DEPTH( CV_8U ) )
	{
		one_chan_gray = input;
	}
	else
	{
		assert( !"Unsupported image format" );
	}

	if ( !one_chan_gray.empty() )
	{
		equalizeHist( one_chan_gray, one_chan_gray );
		Mat gray_float( one_chan_gray.size(), CV_32F );
		one_chan_gray.convertTo( gray_float, CV_32F );

		Mat float_sized( data_height, data_width, CV_32F );
		cv::resize( gray_float, float_sized, float_sized.size() );

		Mat ret( 1, data_height * data_width, CV_32F );
		for ( int mm = 0; mm < data_height; ++mm )
		{
			for ( int kk = 0; kk < data_width; ++kk )
			{
				const int cur_el = mm * data_width + kk;
				float val = float_sized.at< float >( mm, kk );
				val = val / 255.F;
				ret.at< float >( 0, cur_el ) = val;
			}
		}

		return ret;
	}
	return cv::Mat();
}

// read from file and convert to necessary format
inline cv::Mat from_file_to_row( const std::string& file_name )
{
	using namespace std;
	using namespace cv;
	Mat next_mat( imread( file_name.c_str() ) );
	if ( !next_mat.empty() )
	{
		return convert_to_row( next_mat );
	}
	cerr << "Failed read file: " << file_name;
	return Mat();
}

inline std::pair< char, double > proc( const cv::Mat& input )
{
	using namespace cv;
	using namespace std;

	static CvANN_MLP mlp;
	if ( mlp.get_layer_count() == 0 )
	{
		try
		{
			FileStorage fs("C:\\dork\\plate_recognition\\other\\neural_net.yml", cv::FileStorage::READ);
			FileNode fn = fs["mlp"];
			if ( !fn.empty() )
			{
				mlp.read(*fs, *fn);
			}
			else
			{
				throw runtime_error( "invalid data in file" );
			}
		}
		catch ( const exception& e )
		{
			throw runtime_error( string( "Failed load train neural network state, reason: " ) + e.what() );
		}
	}

	Mat pred_out;
	mlp.predict( convert_to_row( input ), pred_out );
	const int max_val = search_max_val( pred_out );
	char ret_char;
	switch ( max_val )
	{
	default:
		assert( !"govno" );
	case 0:
		ret_char = '0';
	case 1:
		ret_char = '1';
	case 2:
		ret_char = '2';
	case 3:
		ret_char = '3';
	case 4:
		ret_char = '4';
	case 5:
		ret_char = '5';
	case 6:
		ret_char = '6';
	case 7:
		ret_char = '7';
	case 8:
		ret_char = '8';
	case 9:
		ret_char = '9';
	}
	float min_diff = 100.;
	for ( int nn = 0; nn < pred_out.cols; ++nn )
	{
		if ( nn != max_val )
		{
			const float curr_diff = abs( pred_out.at< float >( 0, max_val ) - pred_out.at< float >( 0, nn ) );
			if ( curr_diff < min_diff )
			{
				min_diff = curr_diff;
			}
		}
	}
	return make_pair( ret_char, min_diff * 1000 );
//	output = Mat( t_data.size(), 10, CV_32F );

}