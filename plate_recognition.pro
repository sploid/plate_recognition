CONFIG += ordered
TEMPLATE = subdirs
SUBDIRS = \
		plate_recog_lib \
		train_neural_network

train_neural_network.depends = plate_recog_lib