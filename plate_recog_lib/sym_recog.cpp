#include "sym_recog.h"
#include <stdexcept>
#include <opencv2/opencv.hpp>
#include "utils.h"
#include "syms.h"
#include "figure.h"

#include "neural_net_char.cpp"
#include "neural_net_num.cpp"

using namespace cv;
using namespace std;

namespace { namespace aux {
	NeuralNet_MLP mlp_char;
	NeuralNet_MLP mlp_num;
	static set< string > g_region_codes;
} }

int search_max_val( const cv::Mat& data, int row )
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

cv::Mat convert_to_row( const cv::Mat& input )
{
	Mat one_chan_gray;
	if ( input.channels() == 3 && input.depth() == CV_MAT_DEPTH( CV_8U ) )
	{
		Mat gray( input.size(), CV_8U );
		cvtColor( input, one_chan_gray, CV_RGB2GRAY );
	}
	else if ( input.channels() == 1 && input.depth() == CV_MAT_DEPTH( CV_8U ) )
	{
//		one_chan_gray = input.clone();
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
		// растягиваем по цвету
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

cv::Mat from_file_to_row( const std::string& file_name )
{
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


std::pair< char, double > proc_impl( const cv::Mat& input, cv::NeuralNet_MLP& mlp, index_to_char_func i2c )
{
	assert( mlp.get_layer_count() != 0 );
	Mat pred_out;
	mlp.predict( convert_to_row( input ), pred_out );
	const int max_val = search_max_val( pred_out );
	const pair< char, double > ret = make_pair( i2c( max_val ), predict_min_diff( pred_out, max_val ) );
	if ( ret.first >= '7' && ret.first <= '7' )
	{
//		imwrite( next_name( string( "sym" ) + ret.first ), input );
	}
	return ret;
}

std::pair< char, double > proc_char( const cv::Mat& input )
{
	return proc_impl( input, aux::mlp_char, &index_to_char_char );
}

std::pair< char, double > proc_num( const cv::Mat& input )
{
	return proc_impl( input, aux::mlp_num, &index_to_char_num );
}

#include <fstream>

void init_nn( NeuralNet_MLP& mlp, const string& data )
{
	assert( mlp.get_layer_count() == 0 );
	try
	{
		FileStorage fs( data, cv::FileStorage::READ | cv::FileStorage::MEMORY );
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

void init_recognizer()
{
	init_nn( aux::mlp_char, string( neural_net_char, sizeof( neural_net_char ) ) );
	init_nn( aux::mlp_num, string( neural_net_num, sizeof( neural_net_num ) ) );
	aux::g_region_codes.insert( "178" ); // ХЗ
	aux::g_region_codes.insert( "01" ); // Республика Адыгея
	aux::g_region_codes.insert( "02" );
	aux::g_region_codes.insert( "102" ); // Республика Башкортостан
	aux::g_region_codes.insert( "03" ); // Республика Бурятия
	aux::g_region_codes.insert( "04" ); // Республика Алтай (Горный Алтай)
	aux::g_region_codes.insert( "05" ); // Республика Дагестан
	aux::g_region_codes.insert( "06" ); // Республика Ингушетия
	aux::g_region_codes.insert( "07" ); // Кабардино-Балкарская Республика
	aux::g_region_codes.insert( "08" ); // Республика Калмыкия
	aux::g_region_codes.insert( "09" ); // Республика Карачаево-Черкессия
	aux::g_region_codes.insert( "10" ); // Республика Карелия
	aux::g_region_codes.insert( "11" ); // Республика Коми
	aux::g_region_codes.insert( "12" ); // Республика Марий Эл
	aux::g_region_codes.insert( "13" );
	aux::g_region_codes.insert( "113" ); // Республика Мордовия
	aux::g_region_codes.insert( "14" ); // Республика Саха (Якутия)
	aux::g_region_codes.insert( "15" ); // Республика Северная Осетия-Алания
	aux::g_region_codes.insert( "16" );
	aux::g_region_codes.insert( "116" ); // Республика Татарстан
	aux::g_region_codes.insert( "17" ); // Республика Тыва
	aux::g_region_codes.insert( "18" ); // Удмуртская Республика
	aux::g_region_codes.insert( "19" ); // Республика Хакасия
	aux::g_region_codes.insert( "20" ); // утилизировано (бывшая Чечня)
	aux::g_region_codes.insert( "21" );
	aux::g_region_codes.insert( "121" ); // Чувашская Республика
	aux::g_region_codes.insert( "22" ); // Алтайский край
	aux::g_region_codes.insert( "23" );
	aux::g_region_codes.insert( "93" ); // Краснодарский край
	aux::g_region_codes.insert( "24" ); 
	aux::g_region_codes.insert( "84" ); 
	aux::g_region_codes.insert( "88" );
	aux::g_region_codes.insert( "124" ); // Красноярский край
	aux::g_region_codes.insert( "25" );
	aux::g_region_codes.insert( "125" ); // Приморский край
	aux::g_region_codes.insert( "26" ); // Ставропольский край
	aux::g_region_codes.insert( "27" ); // Хабаровский край
	aux::g_region_codes.insert( "28" ); // Амурская область
	aux::g_region_codes.insert( "29" ); // Архангельская область
	aux::g_region_codes.insert( "30" ); // Астраханская область
	aux::g_region_codes.insert( "31" ); // Белгородская область
	aux::g_region_codes.insert( "32" ); // Брянская область
	aux::g_region_codes.insert( "33" ); // Владимирская область
	aux::g_region_codes.insert( "34" ); // Волгоградская область
	aux::g_region_codes.insert( "35" ); // Вологодская область
	aux::g_region_codes.insert( "36" ); // Воронежская область
	aux::g_region_codes.insert( "37" ); // Ивановская область
	aux::g_region_codes.insert( "38" );
	aux::g_region_codes.insert( "85" );
	aux::g_region_codes.insert( "138" ); // Иркутская область
	aux::g_region_codes.insert( "39" );
	aux::g_region_codes.insert( "91" ); // Калининградская область
	aux::g_region_codes.insert( "40" ); // Калужская область
	aux::g_region_codes.insert( "41" );
	aux::g_region_codes.insert( "82" ); // Камчатский край
	aux::g_region_codes.insert( "42" ); // Кемеровская область
	aux::g_region_codes.insert( "43" ); // Кировская область
	aux::g_region_codes.insert( "44" ); // Костромская область
	aux::g_region_codes.insert( "45" ); // Курганская область
	aux::g_region_codes.insert( "46" ); // Курская область
	aux::g_region_codes.insert( "47" ); // Ленинградская область
	aux::g_region_codes.insert( "48" ); // Липецкая область
	aux::g_region_codes.insert( "49" ); // Магаданская область
	aux::g_region_codes.insert( "50" );
	aux::g_region_codes.insert( "90" );
	aux::g_region_codes.insert( "150" );
	aux::g_region_codes.insert( "190" ); // Московская область
	aux::g_region_codes.insert( "51" ); // Мурманская область
	aux::g_region_codes.insert( "52" );
	aux::g_region_codes.insert( "152" ); // Нижегородская область
	aux::g_region_codes.insert( "53" ); // Новгородская область
	aux::g_region_codes.insert( "54" );
	aux::g_region_codes.insert( "154" ); // Новосибирская область
	aux::g_region_codes.insert( "55" ); // Омская область
	aux::g_region_codes.insert( "56" ); // Оренбургская область
	aux::g_region_codes.insert( "57" ); // Орловская область
	aux::g_region_codes.insert( "58" ); // Пензенская область
	aux::g_region_codes.insert( "59" );
	aux::g_region_codes.insert( "81" );
	aux::g_region_codes.insert( "159" ); // Пермский край
	aux::g_region_codes.insert( "60" ); // Псковская область
	aux::g_region_codes.insert( "61" );
	aux::g_region_codes.insert( "161" ); // Ростовская область
	aux::g_region_codes.insert( "62" ); // Рязанская область
	aux::g_region_codes.insert( "63" );
	aux::g_region_codes.insert( "163" ); // Самарская область
	aux::g_region_codes.insert( "64" );
	aux::g_region_codes.insert( "164" ); // Саратовская область
	aux::g_region_codes.insert( "65" ); // Сахалинская область
	aux::g_region_codes.insert( "66" );
	aux::g_region_codes.insert( "96" ); // Свердловская область
	aux::g_region_codes.insert( "67" ); // Смоленская область
	aux::g_region_codes.insert( "68" ); // Тамбовская область
	aux::g_region_codes.insert( "69" ); // Тверская область
	aux::g_region_codes.insert( "70" ); // Томская область
	aux::g_region_codes.insert( "71" ); // Тульская область
	aux::g_region_codes.insert( "72" ); // Тюменская область
	aux::g_region_codes.insert( "73" );
	aux::g_region_codes.insert( "173" ); // Ульяновская область
	aux::g_region_codes.insert( "74" );
	aux::g_region_codes.insert( "174" ); // Челябинская область
	aux::g_region_codes.insert( "75" );
	aux::g_region_codes.insert( "80" ); // Забайкальский край
	aux::g_region_codes.insert( "76" ); // Ярославская область
	aux::g_region_codes.insert( "77" );
	aux::g_region_codes.insert( "97" );
	aux::g_region_codes.insert( "99" );
	aux::g_region_codes.insert( "177" );
	aux::g_region_codes.insert( "199" );
	aux::g_region_codes.insert( "197" ); // г. Москва
	aux::g_region_codes.insert( "78" );
	aux::g_region_codes.insert( "98" );
	aux::g_region_codes.insert( "198" ); // г. Санкт-Петербург
	aux::g_region_codes.insert( "79" ); // Еврейская автономная область
	aux::g_region_codes.insert( "83" ); // Ненецкий автономный округ
	aux::g_region_codes.insert( "86" ); // Ханты-Мансийский автономный округ - Югр
	aux::g_region_codes.insert( "87" ); // Чукотский автономный округ
	aux::g_region_codes.insert( "89" ); // Ямало-Ненецкий автономный округ
	aux::g_region_codes.insert( "92" ); // Резерв МВД Российской Федерации
	aux::g_region_codes.insert( "94" ); // Территории, которые находятся вне РФ и
	aux::g_region_codes.insert( "95" ); // Чеченская республика
}

const std::set< std::string >& region_codes()
{
	return aux::g_region_codes;
}


