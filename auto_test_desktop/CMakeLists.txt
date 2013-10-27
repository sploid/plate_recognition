add_executable(auto_test_desktop main.cpp)
include_directories(${OpenCV_INCLUDE_DIRS} ../plate_recog_lib)
target_link_libraries(auto_test_desktop ${OpenCV_LIBS} plate_recog_lib)
include(../increase_warn_level.cmake)
