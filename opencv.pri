OPENCV_DIR = C:/soft/opencv/build
INCLUDEPATH += $${OPENCV_DIR}/include

win32 {
	QMAKE_CXXFLAGS += /MP
	QMAKE_CXXFLAGS_WARN_ON = -W4
	LIBS += -L$${OPENCV_DIR}/x86/vc11/lib
	CONFIG(debug, debug|release) {
		LIBS += opencv_calib3d246d.lib \
			opencv_contrib246d.lib \
			opencv_core246d.lib \
			opencv_features2d246d.lib \
			opencv_flann246d.lib \
			opencv_gpu246d.lib \
			opencv_highgui246d.lib \
			opencv_imgproc246d.lib \
			opencv_legacy246d.lib \
			opencv_ml246d.lib \
			opencv_nonfree246d.lib \
			opencv_objdetect246d.lib \
			opencv_photo246d.lib \
			opencv_stitching246d.lib \
			opencv_superres246d.lib \
			opencv_ts246d.lib \
			opencv_video246d.lib \
			opencv_videostab246d.lib
	} else {
		LIBS += opencv_calib3d246.lib \
			opencv_contrib246.lib \
			opencv_core246.lib \
			opencv_features2d246.lib \
			opencv_flann246.lib \
			opencv_gpu246.lib \
			opencv_highgui246.lib \
			opencv_imgproc246.lib \
			opencv_legacy246.lib \
			opencv_ml246.lib \
			opencv_nonfree246.lib \
			opencv_objdetect246.lib \
			opencv_photo246.lib \
			opencv_stitching246.lib \
			opencv_superres246.lib \
			opencv_ts246.lib \
			opencv_video246.lib \
			opencv_videostab246.lib
	}
}
