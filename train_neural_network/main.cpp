#include <opencv2/opencv.hpp>

#pragma warning( push )
#pragma warning( disable : 4127 4512 )
#include <QDir>
#pragma warning( pop )
#include <QCoreApplication>

#include "sym_recog.h"

using namespace std;
using namespace cv;

const int max_hidden_neuron = 100;

vector< pair< char, Mat > > train_data( const std::string& image_folder )
{
	vector< pair< char, Mat > > ret;
	QDir image_dir( image_folder.c_str() );
	if ( image_dir.exists() )
	{
		const QStringList all_files = image_dir.entryList( QStringList() << "*.png" );
		for ( int nn = 0; nn < all_files.size(); ++nn )
		{
			const QString& next_file_name = all_files.at( nn );
			const Mat one_chan_gray = from_file_to_row( ( image_dir.absolutePath() + "//" + next_file_name ).toLocal8Bit().data() );
			if ( !one_chan_gray.empty() )
			{
				switch ( next_file_name.toLocal8Bit().data()[ 0 ] )
				{
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					ret.push_back( make_pair( next_file_name.toLocal8Bit().data()[ 0 ], one_chan_gray ) );
				default:
					break;
				}
			}
			else
			{
				cout << "invalid image format: " << next_file_name.toLocal8Bit().data();
			}
		}
	}
	return ret;
}

string path_to_save_train( const string& module_path )
{
	QString q_mod_path( QDir::fromNativeSeparators( QString::fromLocal8Bit( module_path.c_str() ) ) );
	int index_separator = q_mod_path.lastIndexOf( "/" );
	if ( index_separator != -1 )
	{
		q_mod_path.remove( index_separator, q_mod_path.size() - index_separator );
	}
	else
	{
		q_mod_path.clear();
	}
	return string( ( q_mod_path + "/../../other/neural_net.yml" ).toLocal8Bit().data() );
}

void parse_to_input_output_data( const vector< pair< char, Mat > >& t_data, Mat& input, Mat& output, int els_in_row )
{
	input = Mat( 0, els_in_row, CV_32F );
	output = Mat( t_data.size(), 10, CV_32F );
	for ( size_t nn = 0; nn < t_data.size(); ++nn )
	{
		input.push_back( t_data.at( nn ).second );
/*		for ( int mm = 0; mm < t_data.at( nn ).second.rows; ++mm )
		{
			for ( int kk = 0; kk < t_data.at( nn ).second.cols; ++kk )
			{
				const int cur_el = mm * t_data.at( nn ).second.cols + kk;
				input.at< float >( nn, cur_el ) = t_data.at( nn ).second.at< float >( mm, kk );
			}
		}*/
		for ( int mm = 0; mm < 10; ++mm )
		{
			output.at< float >( nn, mm ) = 0.;
		}
		assert( t_data.at( nn ).first >= 48 && t_data.at( nn ).first <= 57 );
		output.at< float >( nn, t_data.at( nn ).first - 48 ) = 1.;
	}
}

float evaluate( const Mat& output, int output_row, const Mat& pred_out )
{
	// проверяем правильный ли ответ
	if ( search_max_val( output, output_row ) != search_max_val( pred_out ) )
		return -10.;

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

int main( int argc, char** argv )
{
	if ( argc <= 1 )
	{
		cout << "usage: auto_test_desktop image_folder";
		return 1;
	}

	// ищем оптимальное количество нейронов и уровней в невидимом слое
	const string image_folder( argv[ 1 ] );
	const int max_hidden_levels = 1;
	const vector< pair< char, Mat > > t_data = train_data( image_folder );
	if ( t_data.empty() )
	{
		cout << "input files not found";
		return 1;
	}
	Mat input, output;
	parse_to_input_output_data( t_data, input, output, data_width * data_height );
	Mat weights( 1, t_data.size(), CV_32FC1, Scalar::all( 1 ) );

	// формируем конфигурации сети
	vector< Mat > configs;
	Mat first_size( 1, 2, CV_32SC1 );
	first_size.at< int >( 0 ) = data_width * data_height;
	first_size.at< int >( 1 ) = 10; // 10 цифр
	configs.push_back( first_size );
	for ( int nn = 1; nn <= max_hidden_levels; ++nn )
	{
		const int all_levels_count = 2 + nn;
		Mat l_size( 1, all_levels_count, CV_32SC1 );
		l_size.at< int >( 0 ) = data_width * data_height;
		l_size.at< int >( all_levels_count - 1 ) = 10; // 10 цифр

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
			CvANN_MLP mlp( configs.at( cc ) );
			mlp.train( input, output, weights );
			for ( size_t ll = 0; ll < t_data.size(); ++ll )
			{
				Mat pred_out;
				mlp.predict( input.row( ll ), pred_out );
				const float diff = evaluate( output, ll, pred_out );
				if ( diff >= -5. )
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
	cout << configs.at( best_index );
	// сохраняем наилучший результат в файл
	CvANN_MLP mlp( configs.at( best_index ) );
	mlp.train( input, output, weights );
	FileStorage fs( path_to_save_train( argv[ 0 ] ), cv::FileStorage::WRITE );
	mlp.write( *fs, "mlp" );
}
