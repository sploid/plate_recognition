#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>

#include <opencv2/opencv.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4127 4512)
#endif

#include <QCoreApplication>
#include <QDir>
#include <QDebug>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "sym_recog.h"

int ParseToInputOutputData(const std::vector<std::pair<char, cv::Mat>>& t_data, cv::Mat& input, cv::Mat& output, int els_in_row) {
  std::set<char> vals; // all chars
  for (auto& next : t_data) {
    vals.insert(next.first);
  }
  int index = 0;
  for (const char& next : vals) {
    qDebug() << "next: " << next << index;
    ++index;
  }
  int count_ret = static_cast<int>(vals.size());
  input = cv::Mat(0, els_in_row, CV_32F);
  output = cv::Mat(static_cast<int>(t_data.size()), count_ret, CV_32F);
  for (size_t nn = 0; nn < t_data.size(); ++nn) {
    input.push_back(t_data.at(nn).second);
    for (int mm = 0; mm < count_ret; ++mm) {
      output.at<float>(static_cast<int>(nn), mm) = 0.;
    }
    const auto cur_iter = find(begin(vals), end(vals), t_data.at(nn).first);
    assert(cur_iter != end(vals));
    const int sym_index = static_cast<int>(std::distance(begin(vals), cur_iter));
    output.at<float>(static_cast<int>(nn), sym_index) = 1.;
  }
  return static_cast<int>(vals.size());
}

float Evaluate(const cv::Mat& output, int output_row, const cv::Mat& pred_out) {
  // проверяем правильный ли ответ
  if (search_max_val(output, output_row) != search_max_val(pred_out))
    return -100.;

  // находим смещение
  float ret = 0.;
  for (int nn = 0; nn < output.cols; ++nn) {
    float diff = output.at<float>(output_row, nn) - pred_out.at<float>(0, nn);
    ret += std::abs(diff);
  }
  return ret;
}

std::vector<std::pair<char, cv::Mat>> ReadTrainData() {
  std::vector<std::pair<char, cv::Mat>> ret;
  if (QCoreApplication::arguments().size() < 2) {
    return ret;
  }
  QDir image_dir(QCoreApplication::arguments()[1]);
  if (image_dir.exists()) {
    const QStringList dir_filter = image_dir.entryList(QDir::Dirs| QDir::NoDotAndDotDot);
    for (QStringList::const_iterator it = dir_filter.constBegin(); it != dir_filter.constEnd(); ++it) {
      if (image_dir.cd(*it)) {
        const QStringList all_files = image_dir.entryList(QStringList() << "*.png");
        for (int nn = 0; nn < all_files.size(); ++nn) {
          const QString& next_file_name = all_files.at(nn);
          const cv::Mat one_chan_gray = from_file_to_row((image_dir.absolutePath() + "//" + next_file_name).toLocal8Bit().data());
          if (!one_chan_gray.empty()) {
            ret.push_back(std::make_pair(it->toLocal8Bit()[0], one_chan_gray));
          } else {
            Q_ASSERT(false);
            std::cout << "invalid image format: " << next_file_name.toLocal8Bit().data();
          }
        }

        image_dir.cdUp();
      }
    }
  }
  return ret;
}

int MakeTraining() {
  if (QCoreApplication::arguments().size() < 3) {
    std::cout << "Invalid output file";
    return -1;
  }

  const std::vector<std::pair<char, cv::Mat>> t_data = ReadTrainData();
  if (t_data.empty()) {
    std::cout << "Input files not found";
    return -1;
  }

  cv::Mat input, output;
  const int count_types_syms = ParseToInputOutputData(t_data, input, output, kDataWidth * kDataHeight);

//  bool have_invalid = false;
//  float diff_summ = 0.;
  try {
    cv::theRNG().state = 0x111111;

    int layer_sz[] = {kDataWidth * kDataHeight, 200, 200, count_types_syms};
    const int nlayers = (int)(sizeof(layer_sz) / sizeof(layer_sz[0]));
    const cv::Mat layer_sizes(1, nlayers, CV_32S, layer_sz);

    cv::Ptr<cv::ml::ANN_MLP> mlp = cv::ml::ANN_MLP::create();
    mlp->setLayerSizes(layer_sizes);
    mlp->setActivationFunction(cv::ml::ANN_MLP::SIGMOID_SYM, 0., 0.);
    mlp->setTermCriteria(cv::TermCriteria(cv::TermCriteria::MAX_ITER + (0 > 0 ? cv::TermCriteria::EPS : 0), 300, 0));
    mlp->setTrainMethod(cv::ml::ANN_MLP::BACKPROP, 0.001);

    cv::Ptr<cv::ml::TrainData> train_data = cv::ml::TrainData::create(input, cv::ml::SampleTypes::ROW_SAMPLE, output);
    const bool train_result = mlp->train(train_data);
    if (!train_result) {
      std::cout << "Failed train.";
      return -1;
    }
    cv::Mat turu;
    mlp->predict(input.row(0), turu);
    {
      cv::FileStorage fs(QCoreApplication::arguments().at(2).toLocal8Bit().data(), cv::FileStorage::WRITE|cv::FileStorage::FORMAT_XML);
      mlp->write(fs);
      fs.release();
    }

/*      for (size_t ll = 0; ll < t_data.size(); ++ll) {
        cv::Mat pred_out;
        float yur = mlp->predict(input.row(ll), pred_out);
        const float diff = Evaluate(output, ll, pred_out);
        if (diff >= -50.) {
          diff_summ += diff;
        } else {
          have_invalid = true;
        }
      }*/
    return 0;
  } catch (const std::exception& e) {
    std::cout << e.what();
    return -1;
  }
/*	cout << configs.at( best_index ) << endl;
	// сохраняем наилучший результат в файл
	theRNG().state = 0x111111;
	CvANN_MLP mlp( configs.at( best_index ) );
	mlp.train( input, output, weights );
	const pair< string, string > file_path = path_to_save_train( num );
	{
		FileStorage fs( file_path.first, cv::FileStorage::WRITE );
		mlp.write( *fs, "mlp" );
		fs.release();
	}
	save_to_cpp( file_path.first, file_path.second, string( "neural_net_" ) + ( num ? "num" : "char" ) );*/
}

int main( int argc, char** argv ) {
  QCoreApplication a( argc, argv );
  return MakeTraining();
}
