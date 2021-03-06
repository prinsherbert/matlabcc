#define MATLABCC_REPORT_STACK

#include <cmath>
#include <iostream>
#include <fstream>

#include <matlabcc>

template<typename T>
inline size_t abcsolve(T &x0, T &x1, T a, T b, T c) {
  T d = b*b - 4*a*c;
  if (d < 0) return 0;
  if (d == 0) {
    x1 = x0 = -b / (2*a);
    return 1;
  }
  d = std::sqrt(d);
  x0 = (-b - d) / (2*a);
  x1 = (-b + d) / (2*a);
  return 2;
}

template<typename T>
inline bool covsolve(T &l0, T &l1, T &a, T &b, T &c, T &ar, T &br, T &cr) {
  size_t n = abcsolve(
    l0, l1,
    a*b - c*c,
    2*cr*c - a*br - ar*b,
    ar*br - cr*cr
  );
  return n!=0;
}

template<typename T>
inline T sqln(T v) {
  T r = std::log(v < 0 ? -v : v);
  r *= r;
  if (v < 0)
    r += 9.86960440109; // (+= pi^2)
  return r;
}

template<typename T>
inline void cov2add(T &v0, T &v1, T &cv, mtb::Mat<T> &im, size_t row, size_t col, T kv, T *mean) {
  v0 += kv * (im(row, col, 1) - mean[1]) * (im(row, col, 1) - mean[1]);
  v1 += kv * (im(row, col, 2) - mean[2]) * (im(row, col, 2) - mean[2]);
  cv += kv * (im(row, col, 1) - mean[1]) * (im(row, col, 2) - mean[2]);
}

template<typename T>
inline T sim(double v0, double v1) {
  return 2*v1*v0 / (v0*v0 + v1*v1);
}

template<typename T>
inline T diff2sim(double v, double max) {
  return std::max(static_cast<T>(0), (max - v) / max);
}

void mexFunction(int nargout, mxArray *argout[], int nargin, const mxArray *argin[])
{
  mtb::Mat<double> im(argin[0]);
  mtb::Mat<double> imreference(argin[1]);
  mtb::Mat<double> kernel(argin[2]);
  
  argout[0] = mtb::create<2, double>(im.size(0), im.size(1));
  mtb::Mat<double, true> colordiff(argout[0]);
  
  argout[1] = mtb::create<2, double>(im.size(0), im.size(1));
  mtb::Mat<double, true> ssim(argout[1]);
  
  size_t kr = kernel.size(0) / 2;
  size_t kc = kernel.size(1) / 2;
  
  mtb::forEachElement<2>(im, [&](size_t row, size_t col) {
    double mean[3];
    double sum = mtb::meanKernel(mean,          im,          row, col, kernel, kr, kc);
    double meanreference[3];
                 mtb::meanKernel(meanreference, imreference, row, col, kernel, kr, kc);
    
    double a  = 0, b  = 0, c  = 0;
    double ar = 0, br = 0, cr = 0;
    double v0 = 0, v1 = 0, cv = 0;
    mtb::forKernel(im, row, col, kernel, kr, kc, [&](double kv, size_t row, size_t col) {
      cov2add(a, b, c,  im,          row, col, kv, mean);
      cov2add(ar,br,cr, imreference, row, col, kv, meanreference);
      
      v0 += kv * (im         (row, col, 0) - mean[0]) * (im         (row, col, 0) - mean[0]);
      v1 += kv * (imreference(row, col, 0) - mean[0]) * (imreference(row, col, 0) - mean[0]);
      cv += kv * (im         (row, col, 0) - mean[0]) * (imreference(row, col, 0) - mean[0]);
      
    });
    a  /= sum; b  /= sum; c  /= sum;
    ar /= sum; br /= sum; cr /= sum;
    
    double l0, l1;
    if (covsolve(l0, l1, a, b, c, ar, br, cr))
      colordiff(row, col) = std::sqrt(sqln(l0) + sqln(l1));
    else
      colordiff(row, col) = 0;
    
    ssim(row, col) = (0.001 + 2*mean[0]*meanreference[0]) / (0.01 + meanreference[0]*meanreference[0] + mean[0]*mean[0]) * (0.03 + 2*cv) / (0.03 + v0 + v1);
  });
}









