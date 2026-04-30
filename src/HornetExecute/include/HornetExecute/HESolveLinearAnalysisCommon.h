#pragma once

#include <eigen3/Eigen/Dense>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <limits>
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace FEMLinearStatic {

using Vec3 = Eigen::Vector3d;
using Vec6 = Eigen::Matrix<double, 6, 1>;
using Vec7 = Eigen::Matrix<double, 7, 1>;
using Mat66 = Eigen::Matrix<double, 6, 6>;

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

} // namespace FEMLinearStatic
