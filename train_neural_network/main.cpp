#include <opencv2/opencv.hpp>

#pragma warning( push )
#pragma warning( disable : 4127 4512 )
#include <QDir>
#pragma warning( pop )
#include <QCoreApplication>

#include "sym_recog.h"

using namespace std;
using namespace cv;

const int max_hidden_neuron = 25;

vector< pair< char, Mat > > train_data( bool num )
{
	const string image_folder( QDir::toNativeSeparators( QCoreApplication::applicationDirPath() + "/../../train_data" ).toLocal8Bit() );

	vector< pair< char, Mat > > ret;
	QDir image_dir( image_folder.c_str() );
	if ( image_dir.exists() )
	{
		QStringList filters;
		if ( num )
		{
			filters << "0*.png" << "1*.png" << "2*.png" << "3*.png" << "4*.png" << "5*.png" << "6*.png" << "7*.png" << "8*.png" << "9*.png";
		}
		else
		{
			filters << "A*.png" << "B*.png" << "C*.png" << "E*.png" << "H*.png" << "K*.png" << "M*.png" << "O*.png" << "P*.png" << "T*.png" << "X*.png" << "Y*.png";
		}
		const QStringList all_files = image_dir.entryList( filters );
		for ( int nn = 0; nn < all_files.size(); ++nn )
		{
			const QString& next_file_name = all_files.at( nn );
			const Mat one_chan_gray = from_file_to_row( ( image_dir.absolutePath() + "//" + next_file_name ).toLocal8Bit().data() );
			if ( !one_chan_gray.empty() )
			{
				ret.push_back( make_pair( next_file_name.toLocal8Bit().data()[ 0 ], one_chan_gray ) );
			}
			else
			{
				cout << "invalid image format: " << next_file_name.toLocal8Bit().data();
			}
		}
	}
	return ret;
}

string path_to_save_train( bool num )
{
	const string nn_config_folder( QDir::toNativeSeparators( QCoreApplication::applicationDirPath() + "/../../other" ).toLocal8Bit() );
	return num ? nn_config_folder + "\\neural_net_num.yml" : nn_config_folder + "\\neural_net_char.yml";
}

void parse_to_input_output_data( const vector< pair< char, Mat > >& t_data, Mat& input, Mat& output, int els_in_row, bool num )
{
	const int count_ret = num ? 10 : 12;
	input = Mat( 0, els_in_row, CV_32F );
	output = Mat( t_data.size(), count_ret, CV_32F );
	for ( size_t nn = 0; nn < t_data.size(); ++nn )
	{
		input.push_back( t_data.at( nn ).second );
		for ( int mm = 0; mm < count_ret; ++mm )
		{
			output.at< float >( nn, mm ) = 0.;
		}
		int el_index = 0;
		if ( num )
		{
			el_index = t_data.at( nn ).first - 48;
		}
		else
		{
			switch (t_data.at( nn ).first)
			{
			default:
				assert( !"there should be no such char" );
			case 'A':
				el_index = 0;
				break;
			case 'B':
				el_index = 1;
				break;
			case 'C':
				el_index = 2;
				break;
			case 'E':
				el_index = 3;
				break;
			case 'H':
				el_index = 4;
				break;
			case 'K':
				el_index = 5;
				break;
			case 'M':
				el_index = 6;
				break;
			case 'O':
				el_index = 7;
				break;
			case 'P':
				el_index = 8;
				break;
			case 'T':
				el_index = 9;
				break;
			case 'X':
				el_index = 10;
				break;
			case 'Y':
				el_index = 11;
				break;
			}
		}
		output.at< float >( nn, el_index ) = 1.;
	}
}

float evaluate( const Mat& output, int output_row, const Mat& pred_out )
{
	// проверяем правильный ли ответ
	if ( search_max_val( output, output_row ) != search_max_val( pred_out ) )
		return -100.;

	// находим смещение
	float ret = 0.;
	for ( int nn = 0; nn < output.cols; ++nn )
	{
		ret += abs( output.at< float >( output_row, nn ) - pred_out.at< float >( 0, nn ) );
	}
	return ret;
}

void fill_hidden_layers( vector< Mat >& configs, Mat& layer_sizes, int layer_index, int count_hidden )
{
	for ( int nn = layer_sizes.at< int >( layer_sizes.cols - 1 ); nn <= max_hidden_neuron; ++nn )
	{
		layer_sizes.at< int >( layer_index + 1 ) = nn;
		if ( layer_index == count_hidden - 1 )
		{
			configs.push_back( layer_sizes.clone() );
		}
		else
		{
			fill_hidden_layers( configs, layer_sizes, layer_index + 1, count_hidden );
		}
	}
}

void make_training( bool num )
{
	const int max_hidden_levels = 1;
	const vector< pair< char, Mat > > t_data = train_data( num );
	if ( t_data.empty() )
	{
		cout << "input files not found";
		return;
	}
	Mat input, output;
	parse_to_input_output_data( t_data, input, output, data_width * data_height, num );
	Mat weights( 1, t_data.size(), CV_32FC1, Scalar::all( 1 ) );

	// формируем конфигурации сети
	vector< Mat > configs;
	Mat first_size( 1, 2, CV_32SC1 );
	first_size.at< int >( 0 ) = data_width * data_height;
	first_size.at< int >( 1 ) = num ? 10 : 12; // 10 цифр, 12 букв
	configs.push_back( first_size );
	for ( int nn = 1; nn <= max_hidden_levels; ++nn )
	{
		const int all_levels_count = 2 + nn;
		Mat l_size( 1, all_levels_count, CV_32SC1 );
		l_size.at< int >( 0 ) = data_width * data_height;
		l_size.at< int >( all_levels_count - 1 ) = num ? 10 : 12; // 10 цифр, 12 букв

		fill_hidden_layers( configs, l_size, 0, nn );
	}

	assert( !configs.empty() );
	int best_index = -1;
	float best_diff = 100.;
	for ( size_t cc = 0; cc < configs.size(); ++cc )
	{
		cout << configs.at( cc ) << endl;
		bool have_invalid = false;
		float diff_summ = 0.;
		try
		{
			theRNG().state = 0x111111;
			CvANN_MLP mlp( configs.at( cc ) );
			mlp.train( input, output, weights );
			for ( size_t ll = 0; ll < t_data.size(); ++ll )
			{
				Mat pred_out;
				mlp.predict( input.row( ll ), pred_out );
				const float diff = evaluate( output, ll, pred_out );
				if ( diff >= -50. )
				{
					diff_summ += diff;
				}
				else
				{
					have_invalid = true;
				}
			}
		}
		catch (const exception& e)
		{
			(void)e;
			have_invalid = true;
		}
		if ( !have_invalid && ( best_index == -1 || best_diff > diff_summ ) )
		{
			best_index = cc;
			best_diff = diff_summ;
		}
	}
	cout << configs.at( best_index ) << endl;
	// сохраняем наилучший результат в файл
	theRNG().state = 0x111111;
	CvANN_MLP mlp( configs.at( best_index ) );
	mlp.train( input, output, weights );
	FileStorage fs( path_to_save_train( num ), cv::FileStorage::WRITE );
	mlp.write( *fs, "mlp" );
}

int main( int argc, char** argv )
{
	QCoreApplication a( argc, argv );
	make_training( false );
	make_training( true );
}
