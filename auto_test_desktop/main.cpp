#include "engine.h"
#include <iostream>
#include <conio.h>

#include <opencv2/opencv.hpp>

#ifdef WIN32
#include <windows.h>
#endif

struct debug_out : public recog_debug_callback
{
	virtual void out_image( const cv::Mat& ) { }
	virtual void out_string( const std::string& ) { }
};

int process_file( const std::string& folder_name, const std::string& file_name )
{
	using namespace std;
	cout << file_name.substr( folder_name.size() + 1, file_name.size() - folder_name.size() ) << "   ";
	const cv::Mat image = cv::imread( file_name.c_str(), CV_LOAD_IMAGE_COLOR);   // Read the file
	if( image.data )
	{
		const int64 begin = cv::getTickCount();
		debug_out rc;
		const pair< string, int > number = read_number( image, &rc );
		cout << number.first << "   " << number.second << "   " << (((double)cv::getTickCount() - begin)/cv::getTickFrequency()) << endl;
		return number.second;
	}
	else
	{
		cout << "FAILED READ FILE" << endl;
		return 0;
	}
}

int main( int argc, char** argv )
{
	using namespace std;

	if ( argc <= 1 )
	{
		cout << "usage: auto_test_desktop image_folder";
		return 1;
	}
	const string image_folder( argv[ 1 ] );
	cout << "\"file\"  \"number\"  \"weight\"  \"time\"" << endl;
	// грузим список картинок
#ifdef WIN32
	WIN32_FIND_DATA find_file_data;
	HANDLE h_find = FindFirstFile( ( image_folder  + "\\*" ).c_str(), &find_file_data );
	if ( INVALID_HANDLE_VALUE == h_find ) 
	{
		cout << "files not found";
		return 1;
	}
	int sum = 0;
	do
	{
		if ( !( find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
		{
			const string next_file_name = image_folder + "\\" + find_file_data.cFileName;
			sum += process_file( image_folder, next_file_name );
		}
	}
	while ( FindNextFile(h_find, &find_file_data) != 0 );
	FindClose( h_find );
	cout << "sum: " << sum << endl;
	_getch();
#else
#endif
	return 0;
}