#pragma once
#include "HESolveCrackPropagationCommon.h"

namespace XFEMCrackPropagation {

struct HESolveCrackPropagationTriangleMesh {
    std::vector<Vec2> points;
    std::vector<Eigen::Vector3i> triangles;
};

struct HESolveCrackPropagationQuadGaussData {
    std::vector<Vec2> points;
    std::vector<double> weights;
};

struct HESolveCrackPropagationTriGaussData {
    std::vector<Vec2> points;
    std::vector<double> weights;
};

double quadArea(const std::vector<Vec2>& vertices);
double triArea(const std::vector<Vec2>& vertices);
double distance(const Vec2& a, const Vec2& b);

Vec2 quadNaturalToGlobal(const Vec2& point, const std::vector<Vec2>& nodes);
Vec2 quadGlobalToNatural(const Vec2& point, const std::vector<Vec2>& nodes, int maxIterations = 30);
std::vector<std::vector<Vec2>> subQuadMesh(const std::vector<Vec2>& nodes, int numberOfMesh);

HESolveCrackPropagationQuadGaussData getQuadGaussData(int order);
HESolveCrackPropagationQuadGaussData getQuadGaussPoint(int numberOfMesh, int order);

HESolveCrackPropagationTriGaussData getTriGaussData(int order);
HESolveCrackPropagationTriGaussData getTriGaussPoint(const std::vector<Vec2>& nodes, int order);
Vec2 triNaturalToGlobal(const Vec2& point, const std::vector<Vec2>& nodes);

double getLevelSet(const std::vector<Vec2>& crack, const Vec2& point);
bool isItIntersect(const std::vector<Vec2>& line, const std::vector<Vec2>& polygon);
std::vector<Vec2> getIntersectionPoints(const std::vector<Vec2>& line, const std::vector<Vec2>& polygon);
std::vector<Vec2> getPolygonOrdinary(const std::vector<Vec2>& firstSetPoints, const std::vector<Vec2>& secondSetPoints);
HESolveCrackPropagationTriangleMesh getTriMesh(const std::vector<Vec2>& polygon);
int getEdgeContainPoint(const Vec2& point, const std::vector<Vec2>& polygon);
std::vector<Vec2> getCrackInPolygon(const std::vector<Vec2>& crack, const std::vector<Vec2>& polygon);

} // namespace XFEMCrackPropagation
