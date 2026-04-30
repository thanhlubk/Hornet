#pragma once
#include "HESolveLinearAnalysisCommon.h"

namespace FEMLinearStatic {

struct HexGaussData {
    std::vector<Vec3> points;
    std::vector<double> weights;
};

struct PrismGaussData {
    std::vector<Vec3> points;
    std::vector<double> weights;
};

double tetVolume(const std::vector<Vec3>& vertices);
double prismVolume(const std::vector<Vec3>& vertices);
HexGaussData getHexGaussData2x2x2();
PrismGaussData getPrismGaussData3x2();

} // namespace FEMLinearStatic
