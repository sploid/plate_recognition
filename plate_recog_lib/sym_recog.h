#pragma once

#include <opencv2/opencv.hpp>

#include "utils.h"

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
		// сглаживаем
		boxFilter( one_chan_gray, one_chan_gray, -1, Size( 3, 3 ) );
		// раст€гиваем по цвету
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

char index_to_char_num( int index )
{
	switch ( index )
	{
	default:
		assert( !"govno" );
	case 0:
		return '0';
	case 1:
		return '1';
	case 2:
		return '2';
	case 3:
		return '3';
	case 4:
		return '4';
	case 5:
		return '5';
	case 6:
		return '6';
	case 7:
		return '7';
	case 8:
		return '8';
	case 9:
		return '9';
	}
}

char index_to_char_char( int index )
{
	switch ( index )
	{
	default:
		assert( !"govno" );
	case 0:
		return 'A';
	case 1:
		return 'B';
	case 2:
		return 'C';
	case 3:
		return 'E';
	case 4:
		return 'H';
	case 5:
		return 'K';
	case 6:
		return 'M';
	case 7:
		return 'O';
	case 8:
		return 'P';
	case 9:
		return 'T';
	case 10:
		return 'X';
	case 11:
		return 'Y';
	}
}

typedef char (*index_to_char_func)( int );

double predict_min_diff( const cv::Mat& pred_out, int max_val )
{
	float ret = 100.; // the current version of the OpenCV Neural Network, the value will not exceed 1.76
	for ( int nn = 0; nn < pred_out.cols; ++nn )
	{
		if ( nn != max_val )
		{
			const float curr_diff = abs( pred_out.at< float >( 0, max_val ) - pred_out.at< float >( 0, nn ) );
			if ( curr_diff < ret )
			{
				ret = curr_diff;
			}
		}
	}
	return ret * 1000.;
}


inline std::pair< char, double > proc_impl( const cv::Mat& input, cv::NeuralNet_MLP& mlp, const std::string& train_file_name, index_to_char_func i2c )
{
	using namespace cv;
	using namespace std;

	if ( mlp.get_layer_count() == 0 )
	{
		try
		{
			FileStorage fs( train_file_name, cv::FileStorage::READ );
			FileNode fn = fs[ "mlp" ];
			if ( !fn.empty() )
			{
				mlp.read( *fs, *fn );
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
	const pair< char, double > ret = make_pair( i2c( max_val ), predict_min_diff( pred_out, max_val ) );
	if ( ret.first == 'A' )
	{
//		imwrite( next_name( string( "sym" ) + ret.first ), input );
	}
	return ret;
}

inline std::pair< char, double > proc_char( const cv::Mat& input )
{
	static cv::NeuralNet_MLP mlp;
	return proc_impl( input, mlp, "C:\\soft\\plate_recognition\\other\\neural_net_char.yml", &index_to_char_char );
}

inline std::pair< char, double > proc_num( const cv::Mat& input )
{
	static cv::NeuralNet_MLP mlp;
	return proc_impl( input, mlp, "C:\\soft\\plate_recognition\\other\\neural_net_num.yml", &index_to_char_num );
}