#include "engine.h"
#include "std_cout_2_qdebug.h"
#include <memory>
#include <iostream>
#include <iomanip>

#include <opencv2/opencv.hpp>

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4127 4512 4244 4251 4800 )
#endif
#include <QCoreApplication>
#include <QDir>
#include <QThreadPool>
#include <QMutex>
#include <QDebug>
#include <QTemporaryFile>
#ifdef QT_GUI_LIB
#include <QApplication>
#include <QDialog>
#include <QPushButton>
#endif // QT_GUI_LIB
#ifdef _MSC_VER
#pragma warning( pop )
#endif

#include "sym_recog.h"

const std::string key_input_folder( "-i" );
const std::string key_output_folder( "-oi" );
const std::string key_output_symbols( "-os" );
const std::string key_output_number( "-on" );

QString get_flag_value( const std::string& key )
{
	const QStringList args = QCoreApplication::arguments();
	const int index_key = args.indexOf( key.c_str() );
	return index_key != -1 && args.size() > index_key + 1 ? args[ index_key + 1 ] : "";
}

bool is_flag_exists( const std::string& key )
{
	const QStringList args = QCoreApplication::arguments();
	return args.indexOf( key.c_str() ) != -1;
}

class process_file_task : public QRunnable
{
public:
	process_file_task( const QString& folder_name, const QString& file_name )
		: m_folder_name( folder_name )
		, m_file_name( file_name )
		, m_output_folder_name( get_flag_value( key_output_folder ) )
		, m_output_number( is_flag_exists( key_output_number ) )
	{
	}

	static int m_sum;

private:
	Q_DISABLE_COPY( process_file_task );

	void run()
	{
		using namespace std;
		stringstream to_out_full, to_out_number;
		const QFileInfo fi( m_file_name );
		const QString file_name_ext( fi.suffix() );
		const QString file_number( fi.baseName() );
		string number_to_out( "[" );
		number_to_out += file_number.toLocal8Bit() + "](test_data/" + fi.fileName().toLocal8Bit() + ")";
		to_out_full << setw( 37 ) << setfill( ' ' ) << number_to_out << "|";
		const cv::Mat image = cv::imread( m_file_name.toLocal8Bit().data(), CV_LOAD_IMAGE_COLOR );   // Read the file
		stringstream size_stream;
		size_stream << image.rows << "x" << image.cols;
		to_out_full << setw( 9 ) << setfill( ' ' ) << size_stream.str() << "|";
		int sum = 0;
		if( image.data )
		{
			const int64 begin = cv::getTickCount();
			const found_number number = read_number( image, 10 );
			if ( !m_output_folder_name.isEmpty() && QDir( m_output_folder_name ).exists() && !number.m_number.empty() )
			{
				// рисуем квадрат номера
				cv::Mat num_rect_image = image.clone();
				for ( size_t nn = 0; nn < number.m_figs.size(); ++nn )
				{
					const figure& cur_fig = number.m_figs[ nn ];
					cv::rectangle( num_rect_image, cv::Point( cur_fig.left(), cur_fig.top() ), cv::Point( cur_fig.right(), cur_fig.bottom() ), CV_RGB( 0, 255, 0 ) );
				}
				cv::imwrite( ( m_output_folder_name + "/" + fi.baseName() + "_out.png" ).toLocal8Bit().data(), num_rect_image );
			}

			to_out_number << number.m_number;
			if ( file_number.toLocal8Bit().data() != number.m_number )
			{
				to_out_full << setw( 13 ) << setfill( ' ' ) << ( string( "~~" ) + number.m_number + "~~" );
			}
			else
			{
				to_out_full << setw( 13 ) << setfill( ' ' ) << number.m_number;
			}
			to_out_full << " - "  << setw( 8 ) << setfill( ' ' ) << setprecision( 5 ) << (((double)cv::getTickCount() - begin)/cv::getTickFrequency()) << "|" << number.m_weight << "   ";
			to_out_full << endl;
			sum = number.m_weight;
		}
		else
		{
			to_out_full << "FAILED READ FILE: " << QFile::exists( m_file_name ) << " " << m_file_name.toLocal8Bit().data();
		}
		static QMutex out_locker;
		QMutexLocker lock( &out_locker );
		if ( m_output_number )
		{
			cout << to_out_number.str();
		}
		else
		{
			cout << to_out_full.str() << endl;
		}
		m_sum += sum;
	}

	const QString m_folder_name;
	const QString m_file_name;
	const QString m_output_folder_name;
	const bool m_output_number;
};

class work_thread : public QThread
{
public:
	work_thread( const QString& image_or_folder )
		: m_image_or_folder( image_or_folder )
	{
	}

protected:
	virtual void run()
	{
		using namespace std;
		stringstream to_out;
		try
		{
			init_recognizer();
			QThreadPool::globalInstance()->setMaxThreadCount( 2 ); // тут минимум 2, т.к. если будет 1, то потоки обработки не будут запускаться
			const int64 begin = cv::getTickCount();
			const QFileInfo fi( m_image_or_folder );
			if ( fi.exists() )
			{
				if ( fi.isFile() )
				{
					QString ss =fi.absolutePath();
					QThreadPool::globalInstance()->start( new process_file_task( fi.absolutePath(), fi.absoluteFilePath() ) );
				}
				else
				{
					// грузим список картинок
					QDir image_dir( fi.absoluteFilePath() );
					if ( image_dir.exists() )
					{
						const QStringList all_files = image_dir.entryList( QStringList() << "*.*", QDir::Files|QDir::Readable );
						for ( int nn = 0; nn < all_files.size(); ++nn )
						{
							const QString next_file_name = image_dir.absolutePath() + "/" + all_files.at( nn );
							QThreadPool::globalInstance()->start( new process_file_task( image_dir.absolutePath(), next_file_name ) );
						}
					}
				}
				QThreadPool::globalInstance()->waitForDone();
				to_out << "sum: " << process_file_task::m_sum << " " << (((double)cv::getTickCount() - begin)/cv::getTickFrequency()) << endl;
			}
			else
			{
				to_out << "Error, folder or file not exists";
			}
		}
		catch ( const std::exception& e )
		{
			to_out << "Catch exeption, what: " << e.what();
		}
		if ( !is_flag_exists( key_output_number ) )
		{
			cout << to_out.str().c_str();
		}
	}
private:
	const QString m_image_or_folder;
};

int process_file_task::m_sum = 0;

void print_help()
{
	using namespace std;
	cout << "Plate number recognition" << endl;
	cout << "Usage: auto_test_desktop " << key_input_folder << " input [" << key_output_folder << " out_im] [-os out_sym]" << endl << endl;
	cout << "Flags:" << endl;
	cout << key_input_folder << "      sets or folder with pictures or a single file for recognition" << endl;
	cout << key_output_folder << "     specifies the target folder for pictures with the result" << endl;
	cout << key_output_symbols << "     specifies the target folder for storing intermediate symbols" << endl;
	cout << key_output_number << "     print only found number" << endl << endl;
}

bool validate_sym_folders_exists( const QString& output_symbol_folder )
{
	QDir dir( output_symbol_folder );
	if ( !dir.exists() )
	{
		// output folder not exists
		return false;
	}

	std::vector< char > all_syms = all_symbols();
	for ( size_t nn = 0; nn < all_syms.size(); ++nn )
	{
		const QString next_sym_dir( all_syms[ nn ] );
		if ( !dir.exists( next_sym_dir )
			&& !dir.mkdir( next_sym_dir ) )
		{
			// failed create sym folder
			return false;
		}
		if ( !dir.cd( next_sym_dir ) )
		{
			// failed cd in sym folder
			return false;
		}
		const QFileInfoList all_files = dir.entryInfoList( QStringList() << "*.png" );
		for ( int nn = 0; nn < all_files.size(); ++nn )
		{
			const QString next_file_name( all_files[ nn ].absoluteFilePath() );
			if ( QFile::exists( next_file_name )
				&& !QFile::remove( next_file_name ) )
			{
				// failed remove symbol file
				return false;
			}
		}
		if ( !dir.cdUp() )
		{
			// failed cd to sym root folder
			return false;
		}
	}

	return true;
}

// @todo: Переписать вывод в классе process_file_task
int main( int argc, char** argv )
{
	using namespace std;
#ifdef ANDROID
	QApplication a( argc, argv );
	std_cout_2_qdebug cc;
	(void)cc;

	const string image_folder( "/storage/sdcard0/test_data" );

	QDialog *dlg = new QDialog();
	QPushButton* btn = new QPushButton( "WAIT", dlg );
	a.connect( btn, SIGNAL( clicked() ), SLOT( quit() ) );
	dlg->show();
	work_thread wt( image_folder.c_str() );
	a.connect( &wt, SIGNAL( finished() ), SLOT( quit() ) );
	wt.start();
	const int ret = a.exec();
	wt.wait();
	return ret;
#else
	QCoreApplication a( argc, argv );
	const QStringList app_args = QCoreApplication::arguments();
	if ( app_args.indexOf( "-h" ) != -1 || app_args.indexOf( "-help" ) != -1 || app_args.indexOf( "--help" ) != -1 || app_args.indexOf( "/?" ) != -1 )
	{
		print_help();
		return 1;
	}
	const QString image_or_folder = get_flag_value( key_input_folder );
	if ( image_or_folder.isEmpty() )
	{
		cout << "Invalid arguments" << endl << endl;
		print_help();
		return 1;
	}
	const QString output_symbol_folder( get_flag_value( key_output_symbols ) );
	if ( !output_symbol_folder.isEmpty() )
	{
		if ( !validate_sym_folders_exists( output_symbol_folder ) )
		{
			cout << "Failed create folders for output symbols" << endl << endl;
			return 1;
		}
		set_output_symbol_folder( output_symbol_folder.toLocal8Bit().data() );
	}
	work_thread wt( image_or_folder );
	a.connect( &wt, SIGNAL( finished() ), SLOT( quit() ) );
	wt.start();
	const int ret = a.exec();
	wt.wait();
	return ret;
#endif // QT_GUI_LIB
}
