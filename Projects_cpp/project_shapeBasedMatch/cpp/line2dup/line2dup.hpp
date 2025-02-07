#ifndef LINE2D_UP_HPP
#define LINE2D_UP_HPP

#include "precomp.hpp"

namespace line2Dup {

#define line2d_eps 1e-7f
#define _degree_(x) ((x)*CV_PI) / 180.0

#define QUANTIZE_BASE 16

#if QUANTIZE_BASE == 16
#define QUANTIZE_TYPE CV_16U
typedef ushort quantize_type;
#elif QUNATIZE_BASE == 8
#define QUANTIZE_TYPE CV_8U
#endif

// Feature -> Gradient -> Candidate

struct Feature {
  int x;
  int y;
  int label;
  Feature() : x(-1), y(-1), label(0) {}
  Feature(int _x, int _y, int _label) : x(_x), y(_y), label(_label) {}

  void read(const cv::FileNode &fn);
  void write(cv::FileStorage &fs) const;
};

#define angle2label(x) (static_cast<int>(x * 32.0 / 360.0) & 15)

struct Gradient : Feature {
  float angle;
  Gradient() : Feature(), angle(0) {}
  Gradient(int _x, int _y, float _angle)
      : Feature(_x, _y, angle2label(_angle)), angle(_angle) {}
  Gradient& operator =(const Gradient &rhs) {
    if (this != &rhs) {
      x = rhs.x;
      y = rhs.y;
      label = rhs.label;
      angle = rhs.angle;
    }
    return *this;
  }
};

/// @brief 待筛选的特征结构体
struct Candidate : Gradient {
  float score;
  Candidate(int _x, int _y, float _angle, float _score)
      : Gradient(_x, _y, _angle), score(_score) {}

  bool operator<(const Candidate &rhs) const { return score > rhs.score; }
};

// ShapeTemplate
class ShapeTemplate {
public:
  cv::RotatedRect box;
  std::vector<Gradient> features;
  int pyramid_level;
  float scale;
  float angle;

  ShapeTemplate(int _pyramid_level, float _scale, float _angle)
    : pyramid_level(_pyramid_level), scale(_scale), angle(fmod(_angle + 360.0f, 360.0f)) {}

  // 加载旋转缩放
  cv::Ptr<ShapeTemplate> relocate(float new_scale, float new_angle);

  // 数据存储与读取
  void read(const cv::FileNode &fn);
  void write(cv::FileStorage &fs) const;
};

/// Search

struct Range {
  float lower_bound;
  float upper_bound;
  float step;
  Range(float l, float u, float s) : lower_bound(l), upper_bound(u), step(s) {}
  Range(float range_params[3])
      : lower_bound(range_params[0]), upper_bound(range_params[1]),
        step(range_params[2]) {}
};

struct Search {
  Range scale;
  Range angle;
  Search() : scale(1, 1, 0), angle(0, 0, 0) {}
  Search(Range _scale, Range _angle) : scale(_scale), angle(_angle) {}
};

/// ColorGradientPyramid

/// @brief 1. 计算梯度方向的量化矩阵 2. 将提取模型
class ColorGradientPyramid {
public:
  ColorGradientPyramid(const cv::Mat &_src, 
                       const cv::Mat &_mask,
                       float _magnitude_threshold = 80.0f, 
                       int count_kernel_size = 5,
                       size_t _num_features = 100);

  cv::Ptr<ColorGradientPyramid> process(const cv::Mat src,
                                        const cv::Mat &mask = cv::Mat()) const {
    return cv::makePtr<ColorGradientPyramid>(src, mask);
  }

  void quantize(cv::Mat &dst) const {
    angle.copyTo(dst, mask);
  }

  bool extractTemplate(ShapeTemplate &templ) const;

  void pyrDown();

private:
  inline void update();

  int pyramid_level;

  cv::Mat src;
  cv::Mat mask;

  cv::Mat magnitude;
  cv::Mat angle;
  cv::Mat quantized_angle;

  float magnitude_threshold;
  int count_kernel_size;
  size_t num_features;
};


/// Match and Detector

struct Match {
  int x;
  int y;
  float similarity;
  cv::String class_id;
  int template_id;
  Match(int _x, int _y, float _similarity, const cv::String &_class_id,
        int _template_id)
      : x(_x), y(_y), similarity(_similarity), class_id(_class_id),
        template_id(_template_id) {}

  bool operator<(const Match &rhs) const {
    if (similarity != rhs.similarity)
      return similarity < rhs.similarity;
    else
      return template_id < rhs.template_id;
  }

  bool operator==(const Match &rhs) const {
    return x == rhs.x && y == rhs.y && similarity == rhs.similarity &&
           class_id == rhs.class_id;
  }
};

class LinearMemory {
public:
  int block_size;
  int rows;
  int cols;

  LinearMemory(int _block_size)
    : block_size(_block_size) { 
    memories = std::vector<std::vector<ushort>>(block_size * block_size, std::vector<ushort>());
  }

  size_t linear_size() { return memories[0].size(); }

  void create(size_t size, ushort value = 0) {
    for (int i = 0; i < (int)memories.size(); i++) 
      memories[i] = std::vector<ushort>(size, value);
  }

  /// @brief memories[i][j] -> linearized Mat S_{orientaion}(c)
  /// @param i -> order in TxT kernel
  /// @param j -> index in linear vector
  /// @return &memories[i][j]
  ushort &at(size_t i, size_t j) {
    static ushort default_value = 0; // 静态的默认值
    if (i < 0 || i >= memories.size())
      return default_value;
    if (j < 0 || j >= memories[0].size())
      return default_value;
    return memories[i][j];
  }

  ushort &linear_at(int r, int c) {
    size_t i = (r % block_size) * block_size + (c % block_size);
    size_t j = (r / block_size) * cols + (c / block_size);
    return at(i, j);
  }
  
  void linearize(cv::Mat &src);

  void unlinearize(cv::Mat &dst);

  void read(cv::FileNode &fn);
  void write(cv::FileStorage &fs) const;

private:
  std::vector<std::vector<ushort>> memories;
};

class Detector {
public:
  void addSource(cv::Mat &src, cv::Mat mask = cv::Mat(), const cv::String &memory_name = "default");

  void addTemplate(cv::Mat &object, cv::Mat object_mask = cv::Mat(), const Search &search = Search(), const cv::String &templ_name = "default");

  void addSearch(Range scale, Range angle, const cv::String &search_name = "default");

  void match(cv::Mat &src, cv::Mat &object, 
             float score_threshold,
             const Search &search = Search(),
             cv::Mat src_mask = cv::Mat(), 
             cv::Mat object_mask = cv::Mat());

  void matchClass(const cv::String &match_name, 
                  const cv::String &search_name, float score_threshold);

  void detectBestMatch(std::vector<cv::Vec6f> &points, std::vector<cv::RotatedRect> &boxs , const cv::String &match_name = "default");

private:
  int pyramid_level;
  int block_size;
  cv::Ptr<ColorGradientPyramid> modality;

  std::map<cv::String, std::vector<cv::Ptr<ShapeTemplate> > > templates_map;
  std::map<cv::String, std::vector<LinearMemory> > memories_map;
  std::map<cv::String, Search> searches_map;
  std::map<cv::String, std::vector<Match> >  matches_map;
};
} // namespace line2Dup

#endif // LINE2D_UP_HPP