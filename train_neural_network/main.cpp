// #include <iostream>
// #include <math.h>
// #include <string>
// #include <opencv2/opencv.hpp>
// 
// using namespace cv;
// using namespace std;
// 
// bool plotSupportVectors=true;
// int numTrainingPoints=200;
// int numTestPoints=2000;
// int size=200;
// int eq=0;
// 
// // accuracy
// float evaluate(cv::Mat& predicted, cv::Mat& actual) {
// 	assert(predicted.rows == actual.rows);
// 	int t = 0;
// 	int f = 0;
// 	for(int i = 0; i < actual.rows; i++) {
// 		float p = predicted.at<float>(i,0);
// 		float a = actual.at<float>(i,0);
// 		if((p >= 0.0 && a >= 0.0) || (p <= 0.0 &&  a <= 0.0)) {
// 			t++;
// 		} else {
// 			f++;
// 		}
// 	}
// 	return (t * 1.0) / (t + f);
// }
// 
// // plot data and class
// void plot_binary(cv::Mat& data, cv::Mat& classes, string name) {
// 	cv::Mat plot(size, size, CV_8UC3);
// 	plot.setTo(cv::Scalar(255.0,255.0,255.0));
// 	for(int i = 0; i < data.rows; i++) {
// 
// 		float x = data.at<float>(i,0) * size;
// 		float y = data.at<float>(i,1) * size;
// 
// 		if(classes.at<float>(i, 0) > 0) {
// 			cv::circle(plot, Point(x,y), 2, CV_RGB(255,0,0),1);
// 		} else {
// 			cv::circle(plot, Point(x,y), 2, CV_RGB(0,255,0),1);
// 		}
// 	}
// 	cv::imshow(name, plot);
// }
// 
// // function to learn
// int f(float x, float y, int equation) {
// 	switch(equation) {
// 	case 0:
// 		return y > sin(x*10) ? -1 : 1;
// 		break;
// 	case 1:
// 		return y > cos(x * 10) ? -1 : 1;
// 		break;
// 	case 2:
// 		return y > 2*x ? -1 : 1;
// 		break;
// 	case 3:
// 		return y > tan(x*10) ? -1 : 1;
// 		break;
// 	default:
// 		return y > cos(x*10) ? -1 : 1;
// 	}
// }
// 
// // label data with equation
// cv::Mat labelData(cv::Mat points, int equation) {
// 	cv::Mat labels(points.rows, 1, CV_32FC1);
// 	for(int i = 0; i < points.rows; i++) {
// 		float x = points.at<float>(i,0);
// 		float y = points.at<float>(i,1);
// 		labels.at<float>(i, 0) = f(x, y, equation);
// 	}
// 	return labels;
// }
// 
// void svm(cv::Mat& trainingData, cv::Mat& trainingClasses, cv::Mat& testData, cv::Mat& testClasses) {
// 	CvSVMParams param = CvSVMParams();
// 
// 	param.svm_type = CvSVM::C_SVC;
// 	param.kernel_type = CvSVM::RBF; //CvSVM::RBF, CvSVM::LINEAR ...
// 	param.degree = 0; // for poly
// 	param.gamma = 20; // for poly/rbf/sigmoid
// 	param.coef0 = 0; // for poly/sigmoid
// 
// 	param.C = 7; // for CV_SVM_C_SVC, CV_SVM_EPS_SVR and CV_SVM_NU_SVR
// 	param.nu = 0.0; // for CV_SVM_NU_SVC, CV_SVM_ONE_CLASS, and CV_SVM_NU_SVR
// 	param.p = 0.0; // for CV_SVM_EPS_SVR
// 
// 	param.class_weights = NULL; // for CV_SVM_C_SVC
// 	param.term_crit.type = CV_TERMCRIT_ITER +CV_TERMCRIT_EPS;
// 	param.term_crit.max_iter = 1000;
// 	param.term_crit.epsilon = 1e-6;
// 
// 	// SVM training (use train auto for OpenCV>=2.0)
// 	CvSVM svm(trainingData, trainingClasses, cv::Mat(), cv::Mat(), param);
// 
// 	cv::Mat predicted(testClasses.rows, 1, CV_32F);
// 
// 	for(int i = 0; i < testData.rows; i++) {
// 		cv::Mat sample = testData.row(i);
// 
// 		float x = sample.at<float>(0,0);
// 		float y = sample.at<float>(0,1);
// 
// 		predicted.at<float>(i, 0) = svm.predict(sample);
// 	}
// 
// 	cout << "Accuracy_{SVM} = " << evaluate(predicted, testClasses) << endl;
// 	plot_binary(testData, predicted, "Predictions SVM");
// 
// 	// plot support vectors
// 	if(plotSupportVectors) {
// 		cv::Mat plot_sv(size, size, CV_8UC3);
// 		plot_sv.setTo(cv::Scalar(255.0,255.0,255.0));
// 
// 		int svec_count = svm.get_support_vector_count();
// 		for(int vecNum = 0; vecNum < svec_count; vecNum++) {
// 			const float* vec = svm.get_support_vector(vecNum);
// 			cv::circle(plot_sv, Point(vec[0]*size, vec[1]*size), 3 , CV_RGB(0, 0, 0));
// 		}
// 		cv::imshow("Support Vectors", plot_sv);
// 	}
// }
// 
// void mlp(cv::Mat& trainingData, cv::Mat& trainingClasses, cv::Mat& testData, cv::Mat& testClasses) {
// 
// 	cv::Mat layers = cv::Mat(4, 1, CV_32SC1);
// 
// 	layers.row(0) = cv::Scalar(2);
// 	layers.row(1) = cv::Scalar(10);
// 	layers.row(2) = cv::Scalar(15);
// 	layers.row(3) = cv::Scalar(1);
// 
// 	CvANN_MLP mlp;
// 	CvANN_MLP_TrainParams params;
// 	CvTermCriteria criteria;
// 	criteria.max_iter = 100;
// 	criteria.epsilon = 0.00001f;
// 	criteria.type = CV_TERMCRIT_ITER | CV_TERMCRIT_EPS;
// 	params.train_method = CvANN_MLP_TrainParams::BACKPROP;
// 	params.bp_dw_scale = 0.05f;
// 	params.bp_moment_scale = 0.05f;
// 	params.term_crit = criteria;
// 
// 	mlp.create(layers);
// 
// 	// train
// 	mlp.train(trainingData, trainingClasses, cv::Mat(), cv::Mat(), params);
// 
// 	cv::Mat response(1, 1, CV_32FC1);
// 	cv::Mat predicted(testClasses.rows, 1, CV_32F);
// 	for(int i = 0; i < testData.rows; i++) {
// 		cv::Mat response(1, 1, CV_32FC1);
// 		cv::Mat sample = testData.row(i);
// 
// 		mlp.predict(sample, response);
// 		predicted.at<float>(i,0) = response.at<float>(0,0);
// 
// 	}
// 
// 	cout << "Accuracy_{MLP} = " << evaluate(predicted, testClasses) << endl;
// 	plot_binary(testData, predicted, "Predictions Backpropagation");
// }
// 
// void knn(cv::Mat& trainingData, cv::Mat& trainingClasses, cv::Mat& testData, cv::Mat& testClasses, int K) {
// 
// 	CvKNearest knn(trainingData, trainingClasses, cv::Mat(), false, K);
// 	cv::Mat predicted(testClasses.rows, 1, CV_32F);
// 	for(int i = 0; i < testData.rows; i++) {
// 		const cv::Mat sample = testData.row(i);
// 		predicted.at<float>(i,0) = knn.find_nearest(sample, K);
// 	}
// 
// 	cout << "Accuracy_{KNN} = " << evaluate(predicted, testClasses) << endl;
// 	plot_binary(testData, predicted, "Predictions KNN");
// 
// }
// 
// void bayes(cv::Mat& trainingData, cv::Mat& trainingClasses, cv::Mat& testData, cv::Mat& testClasses) {
// 
// 	CvNormalBayesClassifier bayes(trainingData, trainingClasses);
// 	cv::Mat predicted(testClasses.rows, 1, CV_32F);
// 	for (int i = 0; i < testData.rows; i++) {
// 		const cv::Mat sample = testData.row(i);
// 		predicted.at<float> (i, 0) = bayes.predict(sample);
// 	}
// 
// 	cout << "Accuracy_{BAYES} = " << evaluate(predicted, testClasses) << endl;
// 	plot_binary(testData, predicted, "Predictions Bayes");
// 
// }
// 
// void decisiontree(cv::Mat& trainingData, cv::Mat& trainingClasses, cv::Mat& testData, cv::Mat& testClasses) {
// 
// 	CvDTree dtree;
// 	cv::Mat var_type(3, 1, CV_8U);
// 
// 	// define attributes as numerical
// 	var_type.at<unsigned int>(0,0) = CV_VAR_NUMERICAL;
// 	var_type.at<unsigned int>(0,1) = CV_VAR_NUMERICAL;
// 	// define output node as numerical
// 	var_type.at<unsigned int>(0,2) = CV_VAR_NUMERICAL;
// 
// 	dtree.train(trainingData,CV_ROW_SAMPLE, trainingClasses, cv::Mat(), cv::Mat(), var_type, cv::Mat(), CvDTreeParams());
// 	cv::Mat predicted(testClasses.rows, 1, CV_32F);
// 	for (int i = 0; i < testData.rows; i++) {
// 		const cv::Mat sample = testData.row(i);
// 		CvDTreeNode* prediction = dtree.predict(sample);
// 		predicted.at<float> (i, 0) = prediction->value;
// 	}
// 
// 	cout << "Accuracy_{TREE} = " << evaluate(predicted, testClasses) << endl;
// 	plot_binary(testData, predicted, "Predictions tree");
// 
// }
// 
// 
// int main() {
// 
// 	cv::Mat trainingData(numTrainingPoints, 2, CV_32FC1);
// 	cv::Mat testData(numTestPoints, 2, CV_32FC1);
// 
// 	cv::randu(trainingData,0,1);
// 	cv::randu(testData,0,1);
// 
// 	cv::Mat trainingClasses = labelData(trainingData, eq);
// 	cv::Mat testClasses = labelData(testData, eq);
// 
// 	plot_binary(trainingData, trainingClasses, "Training Data");
// 	plot_binary(testData, testClasses, "Test Data");
// 
// //	svm(trainingData, trainingClasses, testData, testClasses);
// 	mlp(trainingData, trainingClasses, testData, testClasses);
// //	knn(trainingData, trainingClasses, testData, testClasses, 3);
// //	bayes(trainingData, trainingClasses, testData, testClasses);
// //	decisiontree(trainingData, trainingClasses, testData, testClasses);
// 
// 	cv::waitKey();
// 
// 	return 0;
// }
// 




#include "engine.h"
#include "syms.h"
#include <iostream>
#include <conio.h>

#include <opencv2/opencv.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;
using namespace cv;


struct debug_out : public recog_debug_callback
{
	virtual void out_image( const cv::Mat& ) { }
	virtual void out_string( const std::string& ) { }
};

/*int process_file( const std::string& folder_name, const std::string& file_name )
{
	using namespace std;
	cout << file_name.substr( folder_name.size() + 1, file_name.size() - folder_name.size() - 5 ) << "   ";
	const cv::Mat image = cv::imread( file_name.c_str(), CV_LOAD_IMAGE_COLOR);   // Read the file
	if( image.data )
	{
		const int64 begin = cv::getTickCount();
		debug_out rc;
		const pair< string, int > number = read_number( image, &rc, 10 );
		cout << number.first << "   " << number.second << "   " << (((double)cv::getTickCount() - begin)/cv::getTickFrequency());
		if ( file_name.find( number.first ) == string::npos )
		{
			cout << "  !!  ";
		}
		cout << endl;
		return number.second;
	}
	else
	{
		cout << "FAILED READ FILE" << endl;
		return 0;
	}
}

void fill3(cv::Mat & mat, int row, float val1, float val2, float val3)
{
	mat.at< float >(row, 0) = val1;
	mat.at< float >(row, 1) = val2;
	mat.at< float >(row, 2) = val3;
}

int rrr_count = 3;
*/
void fill_mat( Mat& mat, int row, const vector< vector< float > >& syms )
{
	for ( size_t nn = 0; nn < syms.size(); ++nn )
	{
		for ( size_t mm = 0; mm < syms.at( nn ).size(); ++mm )
		{
			const int col = nn * syms.at( 0 ).size() + mm;
			int ssym = 255 - syms.at( nn ).at( mm );
			double fval = static_cast< float >( ssym ) / 255.;
			if ( fval < 0. || fval > 1. )
			{
				int t = 0;
			}
			mat.at<float>( row, col ) = fval;
		}
	}
}

void ff_mat( Mat& mat, const vector< vector< float > >& syms )
{
	for ( size_t nn = 0; nn < syms.size(); ++nn )
	{
		for ( size_t mm = 0; mm < syms.at( nn ).size(); ++mm )
		{
			float val = syms.at( nn ).at( mm );
			mat.at<float>( nn, mm ) = 255 - val;
		}
	}
}

void fill_mat( Mat& mat, int row )
{
	for ( int nn = 0; nn < 4; ++nn )
	{
		float cur_val = ( nn == row ? 1 : 0 );
		mat.at<float>( row, nn ) = cur_val;
	}
}

float line0[] = { 1., 0.66666669, 0.66666669, 0.66666669, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.66666669, 0.66666669, 1, 1, 0.66666669, 0.66666669, 0.33333334, 0.33333334, 0.33333334, 0, 0, 0, 0.33333334, 0.33333334, 0.66666669, 0.66666669, 1, 0.66666669, 0.33333334, 0.33333334, 0, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.66666669, 0.66666669 };
float line1[] = { 0.66666669, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.66666669, 0.33333334, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.33333334, 0.66666669, 0.33333334, 0., 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334 };
float line2[] = { 1., 1., 1., 1., 0.66666669, 0.66666669, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.66666669, 1., 1., 1., 1., 1., 0.66666669, 0.66666669, 0.33333334, 0.33333334,0, 0.33333334, 0.33333334, 0.66666669, 1., 1., 1., 1., 0.66666669, 0.66666669, 0.33333334, 0.33333334, 0, 0.33333334, 0.33333334, 0.66666669, 0.66666669, 1., 1. };
float line3[] = { 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0.33333334, 0., 0., 0.33333334 };

void fill( Mat& mat, int row, float* data, int size )
{
	for ( int nn = 0; nn < size; ++nn )
	{
		mat.at< float >( row, nn ) = data[ nn ];
	}
}

void fill2( Mat & mat, int row, float val1, float val2 )
{
	mat.at< float >( row, 0 ) = val1;
	mat.at< float >( row, 1 ) = val2;
}

vector< pair< char, Mat > > train_data( const std::string& image_folder, int data_height, int data_width )
{
	vector< pair< char, Mat > > ret;
#ifdef _WIN32
	WIN32_FIND_DATAA find_file_data;
	HANDLE h_find = FindFirstFileA( ( image_folder  + "\\*" ).c_str(), &find_file_data );
	if ( INVALID_HANDLE_VALUE == h_find ) 
	{
		return ret;
	}
	do
	{
		if ( !( find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
		{
			const string next_file_name = image_folder + "\\" + find_file_data.cFileName;
			Mat next_mat( imread( next_file_name ) );
			Mat one_chan_gray;
			if ( next_mat.channels() == 3 && next_mat.depth() == CV_MAT_DEPTH( CV_8U ) )
			{
				Mat gray( next_mat.size(), CV_8U );
				cvtColor( next_mat, one_chan_gray, CV_RGB2GRAY );
			}
			else if ( next_mat.channels() == 1 && next_mat.depth() == CV_MAT_DEPTH( CV_8U ) )
			{
				one_chan_gray = next_mat;
			}

			if ( !one_chan_gray.empty() )
			{
				Mat gray_sized( data_height, data_width, CV_8U );
				cv::resize(one_chan_gray, gray_sized, gray_sized.size() );
				Mat gray_float( data_height, data_width, CV_32F );
				gray_sized.convertTo( gray_float, CV_32F );
				switch ( find_file_data.cFileName[ 0 ] )
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
					ret.push_back( make_pair( find_file_data.cFileName[ 0 ], gray_float ) );
				default:
					break;
				}
			}
			else
			{
				cout << "invalid image format: " << find_file_data.cFileName;
			}
		}
	}
	while ( FindNextFileA(h_find, &find_file_data) != 0 );
	FindClose( h_find );
#else
#endif
	return ret;
}

void parse_to_input_output_data( const vector< pair< char, Mat > >& t_data, Mat& input, Mat& output, int els_in_row )
{
	input = Mat( t_data.size(), els_in_row, CV_32F );
	output = Mat( t_data.size(), 10, CV_32F );
	for ( size_t nn = 0; nn < t_data.size(); ++nn )
	{
		for ( int mm = 0; mm < t_data.at( nn ).second.rows; ++mm )
		{
			for ( int kk = 0; kk < t_data.at( nn ).second.cols; ++kk )
			{
				const int cur_el = mm * t_data.at( nn ).second.cols + kk;
				input.at< float >( nn, cur_el ) = t_data.at( nn ).second.at< float >( mm, kk );
			}
		}
		for ( int mm = 0; mm < 10; ++mm )
		{
			output.at< float >( nn, mm ) = 0.;
		}
		assert( t_data.at( nn ).first >= 48 && t_data.at( nn ).first <= 57 );
		output.at< float >( nn, t_data.at( nn ).first - 48 ) = 1.;
	}
}

int search_max_val( const Mat& data, int row )
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

float evaluate( const Mat& output, int output_row, const Mat& pred_out )
{
	// проверяем правильный ли ответ
	if ( search_max_val( output, output_row ) != search_max_val( pred_out, 0 ) )
		return -1;

	// находим смещение
	float ret = 0.;
	for ( int nn = 0; nn < output.cols; ++nn )
	{
		ret += abs( output.at< float >( output_row, nn ) - pred_out.at< float >( 0, nn ) );
	}
	return ret;
}

const int max_hidden_neuron = 500;
const int data_width = 15;
const int data_height = 22;

Mat create_config( int hidden_levels )
{
	Mat ret( 1, 2 + nn, CV_32SC1 );
	for ( int kk = 1; kk < max_hidden_neuron; ++kk )
	{
/*		ret.at< int >( 0 ) = data_width * data_height;
		int mm = 0;
		for ( ; mm < nn; ++mm )
		{
			ret.at< int >( mm + 1 ) = kk;
		}
		ret.at< int >( mm + 1 ) = 10; // 10 цифр*/
	}
	return ret;
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
	const int max_hidden_levels = 3;
	const vector< pair< char, Mat > > t_data = train_data( image_folder, data_height, data_width );
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
	for ( int nn = 0; nn < max_hidden_levels; ++nn )
	{
		configs.push_back( create_config( nn ) );
	}

	for ( size_t cc = 0; cc < configs.size(); ++cc )
	{
		CvANN_MLP mlp( configs.at( cc ) );
		try
		{
			mlp.train( input, output, weights );
			Mat test_sample( 1, data_width * data_height, CV_32FC1 );
			for ( size_t ll = 0; ll < t_data.size(); ++ll )
			{
				for ( int yy = 0; yy < data_width * data_height; ++yy )
				{
					test_sample.at< float >( 0, yy ) = input.at< float >( ll, yy );
				}
				Mat pred_out;
				mlp.predict( test_sample, pred_out );
				cout << evaluate( output, ll, pred_out ) << endl;
			}
			int t = 0;
		}
		catch (const exception& e)
		{
			(void)e;
		}
	}
}
