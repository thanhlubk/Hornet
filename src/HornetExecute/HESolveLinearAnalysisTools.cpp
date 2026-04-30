#include "HornetExecute/HESolveLinearAnalysisTools.h"

namespace FEMLinearStatic {

double tetVolume(const std::vector<Vec3>& vertices) {
    if (vertices.size() != 4) {
        throw std::invalid_argument("tetVolume expects 4 vertices");
    }
    Eigen::Matrix4d A;
    for (int i = 0; i < 4; ++i) {
        A(i, 0) = 1.0;
        A(i, 1) = vertices[i].x();
        A(i, 2) = vertices[i].y();
        A(i, 3) = vertices[i].z();
    }
    return std::abs(A.determinant()) / 6.0;
}

double prismVolume(const std::vector<Vec3>& vertices) {
    if (vertices.size() != 6) {
        throw std::invalid_argument("prismVolume expects 6 vertices");
    }
    // One consistent decomposition of a linear wedge into 3 tetrahedra.
    return tetVolume({vertices[0], vertices[1], vertices[2], vertices[3]}) +
           tetVolume({vertices[1], vertices[2], vertices[4], vertices[3]}) +
           tetVolume({vertices[2], vertices[4], vertices[5], vertices[3]});
}

HexGaussData getHexGaussData2x2x2() {
    HexGaussData data;
    const double a = 1.0 / std::sqrt(3.0);
    const std::array<double, 2> op = {-a, a};

    for (double xi : op) {
        for (double eta : op) {
            for (double zeta : op) {
                data.points.emplace_back(xi, eta, zeta);
                data.weights.push_back(1.0);
            }
        }
    }
    return data;
}

PrismGaussData getPrismGaussData3x2() {
    PrismGaussData data;

    const std::array<Vec3, 3> triPts = {
        Vec3(1.0 / 6.0, 1.0 / 6.0, 0.0),
        Vec3(2.0 / 3.0, 1.0 / 6.0, 0.0),
        Vec3(1.0 / 6.0, 2.0 / 3.0, 0.0)
    };
    const std::array<double, 3> triW = {
        1.0 / 6.0,
        1.0 / 6.0,
        1.0 / 6.0
    };

    const double a = 1.0 / std::sqrt(3.0);
    const std::array<double, 2> linePts = {-a, a};
    const std::array<double, 2> lineW = {1.0, 1.0};

    for (int k = 0; k < 2; ++k) {
        for (int i = 0; i < 3; ++i) {
            data.points.emplace_back(triPts[i].x(), triPts[i].y(), linePts[k]);
            data.weights.push_back(triW[i] * lineW[k]);
        }
    }
    return data;
}

} // namespace FEMLinearStatic
