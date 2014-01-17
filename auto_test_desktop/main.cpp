#include "engine.h"
#include <iostream>
#include <iomanip>
#include <conio.h>

#include <opencv2/opencv.hpp>

#pragma warning( push )
#pragma warning( disable : 4127 4512 )
#include <QDir>
#pragma warning( pop )
#include <QCoreApplication>
#include <QThreadPool>
#include <QMutex>

#include "sym_recog.h"

class process_file_task : public QRunnable
{
public:
	process_file_task( const std::string& folder_name, const std::string& file_name )
		: m_folder_name( folder_name )
		, m_file_name( file_name )
	{
	}

	static int m_sum;

private:
	Q_DISABLE_COPY( process_file_task );

	void run()
	{
		using namespace std;
		stringstream to_out;
		const string file_name_ext( m_file_name.substr( m_folder_name.size() + 1, m_file_name.size() - m_folder_name.size() - 1 ) );
		const string file_number( file_name_ext.substr( 0, file_name_ext.size() - 4 ) );
		string number_to_out( "[" );
		number_to_out += file_number + "](test_data/" + file_name_ext + ")";
		to_out << setw( 37 ) << setfill( ' ' ) << number_to_out << "|";
		const cv::Mat image = cv::imread( m_file_name.c_str(), CV_LOAD_IMAGE_COLOR );   // Read the file
		stringstream size_stream;
		size_stream << image.rows << "x" << image.cols;
		to_out << setw( 9 ) << setfill( ' ' ) << size_stream.str() << "|";
		int sum = 0;
		if( image.data )
		{
			const int64 begin = cv::getTickCount();
			const pair< string, int > number = read_number( image, 10 );
			if ( file_number != number.first )
			{
				to_out << "~~" << setw( 9 ) << setfill( ' ' ) << number.first << "~~";
			}
			else
			{
				to_out << setw( 13 ) << setfill( ' ' ) << number.first;
			}
			to_out << " - "  << setw( 8 ) << setfill( ' ' ) << setprecision( 5 ) << (((double)cv::getTickCount() - begin)/cv::getTickFrequency()) << "|" << number.second << "   ";
			to_out << endl;
			sum = number.second;
		}
		else
		{
			to_out << "FAILED READ FILE";
		}
		static QMutex out_locker;
		QMutexLocker lock( &out_locker );
		cout << to_out.str();
		m_sum += sum;
	}

	const std::string m_folder_name;
	const std::string m_file_name;
};

int process_file_task::m_sum = 0;

int main( int argc, char** argv )
{
	using namespace std;
	QCoreApplication a( argc, argv );

	if ( argc <= 1 )
	{
		cout << "usage: auto_test_desktop image_folder";
		return 1;
	}

	const string nn_config_folder( QDir::toNativeSeparators( QCoreApplication::applicationDirPath() + "/../../other" ).toLocal8Bit() );

	init_recognizer( nn_config_folder + "\\neural_net_num.yml", nn_config_folder + "\\neural_net_char.yml" );

	const int64 begin = cv::getTickCount();
	const string image_folder( argv[ 1 ] );
	// грузим список картинок
	
	QDir image_dir( image_folder.c_str() );
	if ( image_dir.exists() )
	{
		cout << "\"file\"  \"number\"  \"weight\"  \"time\"" << endl;
		const QStringList all_files = image_dir.entryList( QStringList() << "*.*", QDir::Files|QDir::Readable );
		QThreadPool::globalInstance()->setMaxThreadCount( 1 );
		for ( int nn = 0; nn < all_files.size(); ++nn )
		{
			const string next_file_name = image_folder + "\\" + all_files.at( nn ).toLocal8Bit().data();
			QThreadPool::globalInstance()->start( new process_file_task( image_folder, next_file_name ) );
		}
		QThreadPool::globalInstance()->waitForDone();
		cout << "sum: " << process_file_task::m_sum << " " << (((double)cv::getTickCount() - begin)/cv::getTickFrequency()) << endl;

		cv::waitKey();
	}
	else
	{
		cout << "Error, folder not exists";
	}
	return 0;
}