#==============  Надо задать  =================================================
# путь к библиотеке OpenCV
win32 {
	OPENCV_DIR = C:/soft/opencv427/opencv/build
} android {
	OPENCV_DIR = C:\soft\OpenCV-2.4.9-android-sdk\sdk\native
}
# версия библиотеки OpenCV
OPENCV_VER = 247
#==============================================================================

win32 {
	INCLUDEPATH += C:\soft\opencv310\opencv\build\include
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
		LIBS += -LC:\soft\opencv310\opencv\build\x64\vc14\lib
		CONFIG(debug, debug|release) {
			LIBS += opencv_world310d.lib
		} else {
			LIBS += opencv_world310.lib
		}
	}
} android {
#	message($${ANDROID_TARGET_ARCH})
	CONFIG += mobility
	equals(ANDROID_TARGET_ARCH, armeabi-v7a) {
		INCLUDEPATH += $${OPENCV_DIR}/jni/include
		LIBS += -L$${OPENCV_DIR}/libs/armeabi-v7a \
				-L$${OPENCV_DIR}/3rdparty/libs/armeabi-v7a \
				-lopencv_ml \
#				-lopencv_features2d \
#				-lopencv_objdetect \
#				-lopencv_ts \
#				-lopencv_contrib \
#				-lopencv_calib3d \
				-lopencv_highgui \
				-lopencv_androidcamera \
				-lopencv_imgproc \
				-lopencv_core \
				-ltbb \
				-llibjasper \
				-llibpng \
				-llibjpeg \
				-llibtiff \
				-lIlmImf
	}
} linux:!android {
	LIBS += -lopencv_calib3d \
			-lopencv_contrib \
			-lopencv_core \
			-lopencv_features2d \
			-lopencv_flann \
			-lopencv_gpu \
			-lopencv_highgui \
			-lopencv_imgproc \
			-lopencv_legacy \
			-lopencv_ml \
			-lopencv_nonfree \
			-lopencv_objdetect \
			-lopencv_photo \
			-lopencv_stitching \
			-lopencv_superres \
			-lopencv_ts \
			-lopencv_video \
			-lopencv_videostab
}
