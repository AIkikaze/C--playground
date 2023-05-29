#include "line2d.hpp"
using namespace std;
using namespace cv;

ImagePyramid::ImagePyramid() {
  levels = 0;
  pyramid = vector<Mat>();
}

ImagePyramid::ImagePyramid(const Mat &src, int level_size) {
  levels = level_size;
  buildPyramid(src, levels);
}

const Mat &ImagePyramid::operator[](int index) const {
  CV_Assert(index >= 0 && index < levels);
  return pyramid[index];
}

void ImagePyramid::buildPyramid(const Mat &src, int levels) {
  Mat curImg = src.clone();

  // 裁剪图像到合适尺寸
  int suit_size = 1 << levels;
  int n_rows = (curImg.rows / suit_size) * suit_size;
  int n_cols = (curImg.cols / suit_size) * suit_size;
  curImg = curImg(Rect(0, 0, n_cols, n_rows));

  // 初始化图像金字塔
  pyramid.push_back(curImg);

  // 构建图像金字塔
  for (int i = 0; i < levels - 1; i++) {
    Mat downsampledImg;
    pyrDown(pyramid[i], downsampledImg);
    pyramid.push_back(downsampledImg);
  }
}

shapeInfo_producer::shapeInfo_producer() {
  angle_range[0] = angle_range[1] = 0.0f;
  scale_range[0] = scale_range[1] = 1.0f;
  angle_step = 0.0f;
  scale_step = 0.0f;
  eps = line2d_eps;
}

shapeInfo_producer::shapeInfo_producer(const Mat &input_src, Mat input_mask,
                                       bool padding)
    : shapeInfo_producer() {
  CV_Assert(!input_src.empty());
  CV_Assert(input_mask.empty() || (input_src.size() == input_mask.size()));
  CV_Assert(input_mask.empty() || input_mask.type() == CV_8U);
  // 当前图像已经扩充边界
  if (padding) {
    src = input_src;
    if (input_mask.empty())
      mask = Mat(src.size(), CV_8U, Scalar(255));
    else
      mask = input_mask;
  } else {  // 当前图像未扩充 0 边界
    // 图像在旋转和缩放过程中有效像素到图像中心的最远距离
    int border_max = 1 + (int)sqrt(input_src.rows * input_src.rows +
                                   input_src.cols * input_src.cols);
    // 扩充边界
    copyMakeBorder(input_src, src, border_max - input_src.rows / 2,
                   border_max - input_src.rows / 2,
                   border_max - input_src.cols / 2,
                   border_max - input_src.cols / 2, BORDER_REPLICATE);

    if (input_mask.empty())
      mask = Mat(src.size(), CV_8U, Scalar(255));
    else  // 扩充掩图边界
      copyMakeBorder(input_mask, mask, border_max - input_mask.rows / 2,
                     border_max - input_mask.rows / 2,
                     border_max - input_mask.cols / 2,
                     border_max - input_mask.cols / 2, BORDER_CONSTANT);
  }
}

void shapeInfo_producer::save_config(const shapeInfo_producer &sip,
                                     string path) {
  FileStorage fs(path, FileStorage::WRITE);

  // 储存公有成员参数
  fs << "params"
     << "{";
  fs << "angle_range"
     << "[:" << sip.angle_range[0] << sip.angle_range[1] << "]";
  fs << "scale_range"
     << "[:" << sip.scale_range[0] << sip.scale_range[1] << "]";
  fs << "angle_step" << sip.angle_step;
  fs << "scale_step" << sip.scale_step;
  fs << "eps" << sip.eps;
  fs << "}";

  // 储存日志列表
  fs << "infos"
     << "[";
  for (const auto &info : sip.Infos_constptr()) {
    fs << "{";
    fs << "angle" << info.angle;
    fs << "scale" << info.scale;
    fs << "}";
  }
  fs << "]";

  fs.release();
}

Ptr<shapeInfo_producer> shapeInfo_producer::load_config(const Mat &input_src,
                                                        Mat input_mask,
                                                        bool padding,
                                                        string path) {
  FileStorage fs(path, FileStorage::READ);

  // 用 Ptr 管理并初始化 shapeInfo_producer 对象
  Ptr<shapeInfo_producer> sip =
      makePtr<shapeInfo_producer>(input_src, input_mask, padding);

  // 读取公有成员参数
  FileNode paramsNode = fs["params"];
  sip->angle_range[0] = (float)paramsNode["angle_range"][0];
  sip->angle_range[1] = (float)paramsNode["angle_range"][1];
  sip->scale_range[0] = (float)paramsNode["scale_range"][0];
  sip->scale_range[1] = (float)paramsNode["scale_range"][1];
  sip->angle_step = (float)paramsNode["angle_step"];
  sip->scale_step = (float)paramsNode["scale_step"];
  sip->eps = (float)paramsNode["eps"];

  // 读取日志列表
  vector<Info> &infos = sip->Infos_ptr();
  for (const auto &info : fs["infos"]) {
    infos.push_back(Info((float)info["angle"], (float)info["scale"]));
  }

  fs.release();

  return sip;
}

void shapeInfo_producer::produce_infos() {
  CV_Assert(scale_range[0] < scale_range[1] + eps);
  CV_Assert(angle_range[0] < angle_range[1] + eps);
  if (scale_step < eps) scale_step = 2 * eps;
  if (angle_step < eps) angle_step = 2 * eps;
  for (float scale = scale_range[0]; scale <= scale_range[1] + eps;
       scale += scale_step)
    for (float angle = angle_range[0]; angle <= angle_range[1] + eps;
         angle += angle_step) {
      if (angle > 350) cout << angle << endl;
      Infos.push_back(Info(angle, scale));
    }
}

Mat shapeInfo_producer::affineTrans(const Mat &src, float angle, float scale) {
  Mat dst;
  Point2f center(cvFloor(src.cols / 2.0f), cvFloor(src.rows / 2.0f));
  Mat rotate_mat = getRotationMatrix2D(center, angle, scale);
  warpAffine(src, dst, rotate_mat, src.size());
  return dst;
}

Mat shapeInfo_producer::src_of(const Info &info) {
  return affineTrans(src, info.angle, info.scale);
}

Mat shapeInfo_producer::mask_of(const Info &info) {
  return affineTrans(mask, info.angle, info.scale);
}

const vector<shapeInfo_producer::Info> &shapeInfo_producer::Infos_constptr()
    const {
  return Infos;
}

Template::Template() {
  TemplateParams defaultParams;
  nms_kernel_size = defaultParams.nms_kernel_size;
  scatter_distance = defaultParams.scatter_distance;
  grad_norm = defaultParams.grad_norm;
  num_features = defaultParams.num_features;
  template_created = defaultParams.template_crated;
}

Template::Template(TemplateParams params) {
  nms_kernel_size = params.nms_kernel_size;
  scatter_distance = params.scatter_distance;
  grad_norm = params.grad_norm;
  num_features = params.num_features;
  template_created = params.template_crated;
}

ushort Template::angle2bit(const float &angle) {
  float angle_mod = angle > 180 ? angle - 180 : angle;
  ushort quantized = 0;
  for (int i = 0; i < 16; i++) {
    if (angle_mod <= (float)((i + 1) * 180.0f / 16.0f)) {
      quantized = (1 << i);
      break;
    }
  }
  return quantized;
}

void Template::getOriMat(const cv::Mat &src, Mat &edges, Mat &angles) {
  CV_Assert(!src.empty());
  CV_Assert(src.channels() == 1 || src.channels() == 3);

  /// @variables:
  vector<Mat> imgs(3);
  split(src, imgs);  // 拆分成三个通道
  Mat gx = Mat::zeros(src.size(), CV_32F);
  Mat gy = Mat::zeros(src.size(), CV_32F);
  edges = Mat::zeros(src.size(), CV_32F);
  angles = Mat::zeros(src.size(), CV_32F);
  float max_grad_norm = 0.0f;

  // 计算梯度模长和方向角
  if (src.channels() == 1) {  // src 为单通道图像
    GaussianBlur(imgs[0], imgs[0], Size(5, 5), 1.5);
    Sobel(imgs[0], gx, CV_32F, 1, 0, 3);
    Sobel(imgs[0], gy, CV_32F, 0, 1, 3);
    for (int i = 0; i < edges.rows; i++) {
      for (int j = 0; j < edges.cols; j++) {
        edges.at<float>(i, j) = sqrt(gx.at<float>(i, j) * gx.at<float>(i, j) +
                                     gy.at<float>(i, j) * gy.at<float>(i, j));
        angles.at<float>(i, j) =
            fastAtan2(gy.at<float>(i, j), gx.at<float>(i, j));
        if (max_grad_norm < edges.at<float>(i, j))
          max_grad_norm = edges.at<float>(i, j);
      }
    }
  } else {  // src 为 3 通道图像
    vector<Mat> cedges(3, Mat::zeros(src.size(), CV_32F));
    vector<Mat> cangles(3, Mat::zeros(src.size(), CV_32F));
    for (int k = 0; k < 3; k++) {
      // GaussianBlur(imgs[k], imgs[k], Size(5, 5), 1.5);
      Scharr(imgs[k], gx, CV_32F, 1, 0);
      Scharr(imgs[k], gy, CV_32F, 0, 1);
      for (int i = 0; i < edges.rows; i++) {
        for (int j = 0; j < edges.cols; j++) {
          cedges[k].at<float>(i, j) =
              sqrt(gx.at<float>(i, j) * gx.at<float>(i, j) +
                   gy.at<float>(i, j) * gy.at<float>(i, j));
          cangles[k].at<float>(i, j) =
              fastAtan2(gy.at<float>(i, j), gx.at<float>(i, j));
        }
      }
    }
    // 取 3 通道中最大梯度模长对应的方向为方向角方向 Ori(O, r)
    for (int i = 0; i < edges.rows; i++) {
      for (int j = 0; j < edges.cols; j++) {
        float maxEdge = 0.0f;
        int index = 0;
        for (int k = 0; k < 3; k++) {
          if (cedges[k].at<float>(i, j) > maxEdge) {
            maxEdge = cedges[k].at<float>(i, j);
            index = k;
          }
        }
        if (max_grad_norm < maxEdge) max_grad_norm = maxEdge;
        edges.at<float>(i, j) = maxEdge;
        angles.at<float>(i, j) = cangles[index].at<float>(i, j);
      }
    }
  }

  // 归一化梯度模长矩阵
  for (int i = 0; i < edges.rows; i++) {
    for (int j = 0; j < edges.cols; j++) {
      edges.at<float>(i, j) /= max_grad_norm;
    }
  }
}

void Template::createTemplate(const cv::Mat &src, Template &tp,
                              int kernel_size, float lowest_distance) {
  Mat edges, angles;

  getOriMat(src, edges, angles);
  // Mat edgesImage;
  // normalize(edges, edgesImage, 0, 255, NORM_MINMAX, CV_8U);
  // imshow("edges", edgesImage);
  // waitKey();

  tp.selectFeatures_from(edges, angles, kernel_size);

  tp.scatter(lowest_distance);
}

cv::Ptr<Template> Template::makePtr_from(const cv::Mat &src,
                                         TemplateParams params) {
  Ptr<Template> tp = makePtr<Template>(params);

  for (int i = 0; i < 100; i++) {
    createTemplate(src, *tp, params.nms_kernel_size, params.scatter_distance);
    if (tp->prograds.size() >= tp->num_features) break;
  }
  if (tp->prograds.size() >= tp->num_features)
    tp->template_created = true;
  else {
    cerr << "无法找到的足够的特征点！" << endl;
    tp->prograds.clear();
  }

  return tp;
}

void Template::selectFeatures_from(const cv::Mat &_edges,
                                   const cv::Mat &_angles, int kernel_size) {
  Mat edges = _edges.clone();
  Mat angles = Mat::zeros(_angles.size(), CV_8U);

  // 按方向角度进行非极大值抑制
  for (int i = 0; i < angles.rows; i++) {
    for (int j = 0; j < angles.cols; j++) {
      // 取方向角 0 ~ 360
      double angle = _angles.at<float>(i, j);

      // 方向角度量化
      if ((angle > 0 && angle < 22.5) || (angle > 157.5 && angle < 202.5) ||
          (angle > 337.5 && angle < 360))
        angles.at<uchar>(i, j) = 0;
      else if ((angle > 22.5 && angle < 67.5) ||
               (angle > 202.5 && angle < 247.5))
        angles.at<uchar>(i, j) = 45;
      else if ((angle > 67.5 && angle < 112.5) ||
               (angle > 247.5 && angle < 292.5))
        angles.at<uchar>(i, j) = 90;
      else if ((angle > 112.5 && angle < 157.5) ||
               (angle > 292.5 && angle < 337.5))
        angles.at<uchar>(i, j) = 135;
      else
        angles.at<uchar>(i, j) = 0;
    }
  }

  // 非极大值抑制
  Range rows(kernel_size / 2, kernel_size / 2 + edges.rows);
  Range cols(kernel_size / 2, kernel_size / 2 + edges.cols);
  copyMakeBorder(edges, edges, kernel_size / 2, kernel_size / 2,
                 kernel_size / 2, kernel_size / 2, BORDER_CONSTANT, Scalar(0));
  for (int i = rows.start; i < rows.end; i++) {
    for (int j = cols.start; j < cols.end; j++) {
      float prevPixel, nextPixel;
      switch ((int)angles.at<uchar>(i - rows.start, j - cols.start)) {
        case 0:
          prevPixel = edges.at<float>(i, j - 1);
          nextPixel = edges.at<float>(i, j + 1);
          break;
        case 45:
          prevPixel = edges.at<float>(i - 1, j - 1);
          nextPixel = edges.at<float>(i + 1, j + 1);
          break;
        case 90:
          prevPixel = edges.at<float>(i - 1, j);
          nextPixel = edges.at<float>(i + 1, j);
          break;
        case 135:
          prevPixel = edges.at<float>(i + 1, j - 1);
          nextPixel = edges.at<float>(i - 1, j + 1);
          break;
      }

      // 当前点的梯度与梯度方向上临近点的梯度大小
      if (edges.at<float>(i, j) < prevPixel ||
          edges.at<float>(i, j) < nextPixel)
        edges.at<float>(i, j) = 0.0f;
    }
  }
  // Mat edgesImage;
  // normalize(edges, edgesImage, 0, 255, NORM_MINMAX, CV_8U);
  // namedWindow("edges after nms");
  // imshow("edges after nms", edges);
  // waitKey();
  // namedWindow("angles after quantized");
  // imshow("angles after quantized", angles);
  // waitKey();

  // 取核方阵中的梯度值最大的点为特征点
  for (int i = rows.start; i < rows.end; i++) {
    for (int j = cols.start; j < cols.end; j++) {
      if (edges.at<float>(i, j) < grad_norm) continue;
      for (int dy = -kernel_size / 2; dy < kernel_size / 2 + 1; dy++) {
        if (edges.at<float>(i, j) < grad_norm) break;
        for (int dx = -kernel_size / 2; dx < kernel_size / 2 + 1; dx++) {
          if (edges.at<float>(i, j) < edges.at<float>(i + dy, j + dx)) {
            edges.at<float>(i, j) = 0.0f;
            break;
          }
        }
      }
      if (edges.at<float>(i, j) > grad_norm) {
        prograds.push_back(Features(
            Point(j - cols.start, i - rows.start),
            _angles.at<float>(i - rows.start, j - cols.start),
            _edges.at<float>(i, j)));
      }
    }
  }

}

void Template::scatter(float lowest_distance) {
  CV_Assert(lowest_distance > 2.0f);
  CV_Assert(!prograds.empty());

  stable_sort(prograds.begin(), prograds.end());

  lowest_distance = fmax(nms_kernel_size * prograds.size() / num_features, lowest_distance);
  int turns = 0;
  float low_distsq = lowest_distance * lowest_distance;
  float dist = 0.0f;
  vector<Features> new_pg_list;
  new_pg_list.push_back(prograds[0]);

  while (1) {
    turns++;
    for (size_t i = 1; i < prograds.size(); i++) {
      if (new_pg_list.size() >= num_features) break;
      for (size_t j = 0; j < new_pg_list.size(); j++) {
        dist = pow(prograds[i].p_xy.x - new_pg_list[j].p_xy.x, 2) +
               pow(prograds[i].p_xy.y - new_pg_list[j].p_xy.y, 2);
        if (dist > low_distsq) {
          new_pg_list.push_back(prograds[i]);
          break;
        }
      }
    }
    if (turns == 1 && new_pg_list.size() >= num_features) {
      turns = 0;
      lowest_distance += 1.0f;
      low_distsq = lowest_distance * lowest_distance;
      new_pg_list.clear();
      new_pg_list.push_back(prograds[0]);
    } else {  // turns > 1 or new_pg_list.size() < num_features
      lowest_distance -= 1.0f;
      low_distsq = lowest_distance * lowest_distance;
      // 已经选取了足够多的特征点( and turns > 1 )
      if (new_pg_list.size() >= num_features) {
        break;
      } else if (turns > 1 &&
                 lowest_distance <= 2.0f) {  // and new_pg_list.size() <
                                             // num_features
        cerr << "在最小间距下，仍然无法取到足够多的特征点！" << endl;
        break;
      }
      // turns > 1 and new_pg_list.size() < num_features and
      // lowest_disttance > 2.0f turns = 1 and new_pg_list.size() <
      // num_features
    }
  }

  prograds = new_pg_list;
}