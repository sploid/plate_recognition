#==============  Надо задать  =================================================
# путь к библиотеке OpenCV
OPENCV_DIR = C:/soft/opencv427/opencv/build
# версия библиотеки OpenCV
OPENCV_VER = 247
#==============================================================================


INCLUDEPATH += $${OPENCV_DIR}/include

win32 {
	mingw {
		LIBS += -L$${OPENCV_DIR}/x86/mingw/lib \
			-lopencv_calib3d$${OPENCV_VER}.dll \
			-lopencv_contrib$${OPENCV_VER}.dll \
			-lopencv_core$${OPENCV_VER}.dll \
			-lopencv_features2d$${OPENCV_VER}.dll \
			-lopencv_flann$${OPENCV_VER}.dll \
			-lopencv_gpu$${OPENCV_VER}.dll \
			-lopencv_highgui$${OPENCV_VER}.dll \
			-lopencv_imgproc$${OPENCV_VER}.dll \
			-lopencv_legacy$${OPENCV_VER}.dll \
			-lopencv_ml$${OPENCV_VER}.dll \
			-lopencv_nonfree$${OPENCV_VER}.dll \
			-lopencv_objdetect$${OPENCV_VER}.dll \
			-lopencv_photo$${OPENCV_VER}.dll \
			-lopencv_stitching$${OPENCV_VER}.dll \
			-lopencv_superres$${OPENCV_VER}.dll \
			-lopencv_ts$${OPENCV_VER} \
			-lopencv_video$${OPENCV_VER}.dll \
			-lopencv_videostab$${OPENCV_VER}.dll
	} else {
		QMAKE_CXXFLAGS += /MP
		QMAKE_CXXFLAGS_WARN_ON = -W4
		LIBS += -L$${OPENCV_DIR}/x86/vc11/lib
		CONFIG(debug, debug|release) {
			LIBS += opencv_calib3d$${OPENCV_VER}d.lib \
				opencv_contrib$${OPENCV_VER}d.lib \
				opencv_core$${OPENCV_VER}d.lib \
				opencv_features2d$${OPENCV_VER}d.lib \
				opencv_flann$${OPENCV_VER}d.lib \
				opencv_gpu$${OPENCV_VER}d.lib \
				opencv_highgui$${OPENCV_VER}d.lib \
				opencv_imgproc$${OPENCV_VER}d.lib \
				opencv_legacy$${OPENCV_VER}d.lib \
				opencv_ml$${OPENCV_VER}d.lib \
				opencv_nonfree$${OPENCV_VER}d.lib \
				opencv_objdetect$${OPENCV_VER}d.lib \
				opencv_photo$${OPENCV_VER}d.lib \
				opencv_stitching$${OPENCV_VER}d.lib \
				opencv_superres$${OPENCV_VER}d.lib \
				opencv_ts$${OPENCV_VER}d.lib \
				opencv_video$${OPENCV_VER}d.lib \
				opencv_videostab$${OPENCV_VER}d.lib
		} else {
			LIBS += opencv_calib3d$${OPENCV_VER}.lib \
				opencv_contrib$${OPENCV_VER}.lib \
				opencv_core$${OPENCV_VER}.lib \
				opencv_features2d$${OPENCV_VER}.lib \
				opencv_flann$${OPENCV_VER}.lib \
				opencv_gpu$${OPENCV_VER}.lib \
				opencv_highgui$${OPENCV_VER}.lib \
				opencv_imgproc$${OPENCV_VER}.lib \
				opencv_legacy$${OPENCV_VER}.lib \
				opencv_ml$${OPENCV_VER}.lib \
				opencv_nonfree$${OPENCV_VER}.lib \
				opencv_objdetect$${OPENCV_VER}.lib \
				opencv_photo$${OPENCV_VER}.lib \
				opencv_stitching$${OPENCV_VER}.lib \
				opencv_superres$${OPENCV_VER}.lib \
				opencv_ts$${OPENCV_VER}.lib \
				opencv_video$${OPENCV_VER}.lib \
				opencv_videostab$${OPENCV_VER}.lib
		}
	}
}
