#pragma once
#include <eigen3/Eigen/Dense>
#include <algorithm>
#include <cmath>
#include <array>
#include <cstddef>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace xfem {

using Vec2 = Eigen::Vector2d;
using Vec3 = Eigen::Vector3d;
using Vec4 = Eigen::Vector4d;
using Mat22 = Eigen::Matrix2d;
using Mat23 = Eigen::Matrix<double, 2, 3>;
using Mat32 = Eigen::Matrix<double, 3, 2>;
using Mat34 = Eigen::Matrix<double, 3, 4>;
using Mat33 = Eigen::Matrix3d;
using Mat28 = Eigen::Matrix<double, 2, 8>;
using Mat38 = Eigen::Matrix<double, 3, 8>;

constexpr double kEps = 1e-10;

inline bool almostEqual(double a, double b, double eps = 1e-8) {
    return std::abs(a - b) <= eps;
}

inline double safeSqrt(double x) {
    return std::sqrt(std::max(0.0, x));
}

inline std::string trim(std::string s) {
    auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    return s;
}

} // namespace xfem
