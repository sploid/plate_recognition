#include <jni.h>
#include <opencv2/opencv.hpp>
#include "../../../plate_recog_lib/engine.h"
#include "../../../plate_recog_lib/sym_recog.h"
#include "../../../plate_recog_lib/std_cout_2_qdebug.h"
#include <vector>

using namespace std;
using namespace cv;

extern "C" {
JNIEXPORT void JNICALL Java_ru_sploid_platerecog_RecogActivity_InitRecognizer( JNIEnv*, jobject )
{
	std_cout_2_qdebug* cout_redir = new std_cout_2_qdebug();
	init_recognizer();
}

JNIEXPORT void JNICALL Java_ru_sploid_platerecog_RecogActivity_FindFeatures(JNIEnv*, jobject, jlong addr_rgba)
{
    Mat& m_rgba = *(Mat*)addr_rgba;

	try
	{
		const pair< string, int > fn = read_number( m_rgba, 10 );
		cv::putText( m_rgba, fn.first.empty() ? string( "not found" ) : fn.first, cv::Point( 20, 100 ), CV_FONT_HERSHEY_PLAIN, 2.0, cv::Scalar( 255, 0, 0, 0 ) );
	}
	catch ( const std::exception& e )
	{
		cout << "Catch exception: " << e.what() << endl;
		cv::putText( m_rgba, "Exception", cv::Point( 20, 100 ), CV_FONT_HERSHEY_PLAIN, 2.0, cv::Scalar( 255, 0, 0, 0 ) );
	}
}
}
