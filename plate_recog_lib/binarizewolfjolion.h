#pragma once

#include <opencv2/opencv.hpp>

enum NiblackVersion {
  NIBLACK = 0,
  SAUVOLA,
  WOLFJOLION,
};

#define BINARIZEWOLF_DEFAULTDR 128.

void NiblackSauvolaWolfJolion(cv::Mat im, cv::Mat output, NiblackVersion version, int winx, int winy, double k, double dR = BINARIZEWOLF_DEFAULTDR);

