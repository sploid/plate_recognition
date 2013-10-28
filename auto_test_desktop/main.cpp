#include "engine.h"
#include <iostream>
#include <conio.h>

#include <opencv2/opencv.hpp>

#ifdef WIN32
#include <windows.h>
#endif

void process_file( const std::string& folder_name, const std::string& file_name )
{
	using namespace std;
	cout << file_name.substr( folder_name.size() + 1, file_name.size() - folder_name.size() ) << "   ";
	const cv::Mat image = cv::imread( file_name.c_str(), CV_LOAD_IMAGE_COLOR);   // Read the file
	if( image.data )
	{
		const int64 begin = cv::getTickCount();
		const pair< string, int > number = read_number( image );
		cout << number.first << "   " << number.second << "   " << (((double)cv::getTickCount() - begin)/cv::getTickFrequency()) << endl;
	}
	else
	{
		cout << "FAILED READ FILE" << endl;
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
	do
	{
		if ( !( find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
		{
			const string next_file_name = image_folder + "\\" + find_file_data.cFileName;
			process_file( image_folder, next_file_name );
		}
	}
	while ( FindNextFile(h_find, &find_file_data) != 0 );
	FindClose( h_find );
	_getch();
#else
#endif
	return 0;
}