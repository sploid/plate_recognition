CONFIG += ordered
TEMPLATE = subdirs
SUBDIRS = \
		plate_recog_lib \
		auto_test_desktop \
		train_neural_network

auto_test_desktop.depends = plate_recog_lib
qt_gui_test.depends = plate_recog_lib
train_neural_network.depends = plate_recog_lib