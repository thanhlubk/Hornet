#pragma once
#include "HESolverXFEM2DCommon.h"

namespace xfem {

struct HESolverXFEM2DTriangleMesh {
    std::vector<Vec2> points;
    std::vector<Eigen::Vector3i> triangles;
};

struct HESolverXFEM2DQuadGaussData {
    std::vector<Vec2> points;
    std::vector<double> weights;
};

struct HESolverXFEM2DTriGaussData {
    std::vector<Vec2> points;
    std::vector<double> weights;
};

double quadArea(const std::vector<Vec2>& vertices);
double triArea(const std::vector<Vec2>& vertices);
double distance(const Vec2& a, const Vec2& b);

Vec2 quadNaturalToGlobal(const Vec2& point, const std::vector<Vec2>& nodes);
Vec2 quadGlobalToNatural(const Vec2& point, const std::vector<Vec2>& nodes, int maxIterations = 30);
std::vector<std::vector<Vec2>> subQuadMesh(const std::vector<Vec2>& nodes, int numberOfMesh);

HESolverXFEM2DQuadGaussData getQuadGaussData(int order);
HESolverXFEM2DQuadGaussData getQuadGaussPoint(int numberOfMesh, int order);

HESolverXFEM2DTriGaussData getTriGaussData(int order);
HESolverXFEM2DTriGaussData getTriGaussPoint(const std::vector<Vec2>& nodes, int order);
Vec2 triNaturalToGlobal(const Vec2& point, const std::vector<Vec2>& nodes);

double getLevelSet(const std::vector<Vec2>& crack, const Vec2& point);
bool isItIntersect(const std::vector<Vec2>& line, const std::vector<Vec2>& polygon);
std::vector<Vec2> getIntersectionPoints(const std::vector<Vec2>& line, const std::vector<Vec2>& polygon);
std::vector<Vec2> getPolygonOrdinary(const std::vector<Vec2>& firstSetPoints, const std::vector<Vec2>& secondSetPoints);
HESolverXFEM2DTriangleMesh getTriMesh(const std::vector<Vec2>& polygon);
int getEdgeContainPoint(const Vec2& point, const std::vector<Vec2>& polygon);
std::vector<Vec2> getCrackInPolygon(const std::vector<Vec2>& crack, const std::vector<Vec2>& polygon);

} // namespace xfem
