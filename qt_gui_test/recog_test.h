#ifndef COOLSOFT_H
#define COOLSOFT_H

#ifdef WIN32
#pragma warning( push )
#pragma warning( disable : 4127 )
#pragma warning( disable : 4512 )
#endif

#include <QDialog>
#include <QDebug>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include "ui_recog_test.h"

#ifdef WIN32
#pragma warning( pop )
#endif

#include <opencv2/opencv.hpp>
#include "engine.h"

class recog_test : public QDialog, public recog_debug_callback
{
	Q_OBJECT

public:
	recog_test( QWidget *parent );
	static cv::Mat QImage2cvMat( const QImage& qImage );
	static QImage Mat2QImage( const cv::Mat& iplImage );
	void out_image( const cv::Mat & m );
	void out_string( const std::string& text );

private Q_SLOTS:
	void on_m_but_run_clicked();
	void on_m_but_left_clicked();
	void on_m_but_right_clicked();
	void on_m_but_load_image_clicked();
	void on_m_but_remove_cur_image_clicked();

private:
	Ui::recog_test_class ui;
	void update_image();
	QImage cur_image();

	QList< QImage > m_images;
	int m_cur_image;
};

#endif // COOLSOFT_H
