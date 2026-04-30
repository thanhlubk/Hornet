#include "HornetExecute/HESolveCrackPropagationTools.h"

namespace XFEMCrackPropagation {
namespace {

double cross2(const Vec2& a, const Vec2& b) {
    return a.x() * b.y() - a.y() * b.x();
}

double cross2(const Vec2& a, const Vec2& b, const Vec2& c) {
    return cross2(b - a, c - a);
}

bool pointOnSegment(const Vec2& p, const Vec2& a, const Vec2& b, double eps = 1e-8) {
    if (std::abs(cross2(a, b, p)) > eps) {
        return false;
    }
    const double dot = (p - a).dot(p - b);
    return dot <= eps;
}

std::optional<Vec2> segmentIntersection(const Vec2& p1, const Vec2& p2, const Vec2& q1, const Vec2& q2) {
    const Vec2 r = p2 - p1;
    const Vec2 s = q2 - q1;
    const double denom = cross2(r, s);
    const double numer1 = cross2(q1 - p1, s);
    const double numer2 = cross2(q1 - p1, r);

    if (std::abs(denom) < 1e-12) {
        if (std::abs(numer2) < 1e-12) {
            for (const auto& cand : {p1, p2, q1, q2}) {
                if (pointOnSegment(cand, p1, p2) && pointOnSegment(cand, q1, q2)) {
                    return cand;
                }
            }
        }
        return std::nullopt;
    }

    const double t = numer1 / denom;
    const double u = numer2 / denom;
    if (t >= -1e-8 && t <= 1.0 + 1e-8 && u >= -1e-8 && u <= 1.0 + 1e-8) {
        return p1 + t * r;
    }
    return std::nullopt;
}

bool pointInPolygon(const Vec2& point, const std::vector<Vec2>& polygon) {
    bool inside = false;
    for (std::size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
        const auto& pi = polygon[i];
        const auto& pj = polygon[j];
        if (pointOnSegment(point, pj, pi)) {
            return true;
        }
        const bool intersect = ((pi.y() > point.y()) != (pj.y() > point.y())) &&
            (point.x() < (pj.x() - pi.x()) * (point.y() - pi.y()) / ((pj.y() - pi.y()) + 1e-30) + pi.x());
        if (intersect) {
            inside = !inside;
        }
    }
    return inside;
}

double polygonSignedArea(const std::vector<Vec2>& polygon) {
    double sum = 0.0;
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        const auto& a = polygon[i];
        const auto& b = polygon[(i + 1) % polygon.size()];
        sum += a.x() * b.y() - b.x() * a.y();
    }
    return 0.5 * sum;
}

bool isConvex(const Vec2& prev, const Vec2& curr, const Vec2& next, bool ccw) {
    const double c = cross2(prev, curr, next);
    return ccw ? c > 1e-10 : c < -1e-10;
}

bool pointInTriangle(const Vec2& p, const Vec2& a, const Vec2& b, const Vec2& c) {
    const double c1 = cross2(a, b, p);
    const double c2 = cross2(b, c, p);
    const double c3 = cross2(c, a, p);
    const bool hasNeg = (c1 < -1e-10) || (c2 < -1e-10) || (c3 < -1e-10);
    const bool hasPos = (c1 > 1e-10) || (c2 > 1e-10) || (c3 > 1e-10);
    return !(hasNeg && hasPos);
}

void uniquePush(std::vector<Vec2>& pts, const Vec2& p, double eps = 1e-8) {
    for (const auto& item : pts) {
        if ((item - p).norm() <= eps) {
            return;
        }
    }
    pts.push_back(p);
}

double pointLineDistance(const Vec2& p, const Vec2& a, const Vec2& b) {
    const Vec2 ab = b - a;
    const double t = std::clamp((p - a).dot(ab) / (ab.squaredNorm() + 1e-30), 0.0, 1.0);
    const Vec2 proj = a + t * ab;
    return (p - proj).norm();
}

} // namespace

double quadArea(const std::vector<Vec2>& vertices) {
    if (vertices.size() != 4) {
        throw std::invalid_argument("quadArea expects 4 vertices");
    }
    const double diagonal1 = vertices[0].x() * vertices[1].y() + vertices[1].x() * vertices[2].y() +
                             vertices[2].x() * vertices[3].y() + vertices[3].x() * vertices[0].y();
    const double diagonal2 = vertices[1].x() * vertices[0].y() + vertices[2].x() * vertices[1].y() +
                             vertices[3].x() * vertices[2].y() + vertices[0].x() * vertices[3].y();
    return std::abs(0.5 * (diagonal1 - diagonal2));
}

double triArea(const std::vector<Vec2>& vertices) {
    if (vertices.size() != 3) {
        throw std::invalid_argument("triArea expects 3 vertices");
    }
    return 0.5 * std::abs(vertices[0].x() * (vertices[1].y() - vertices[2].y()) +
                          vertices[1].x() * (vertices[2].y() - vertices[0].y()) +
                          vertices[2].x() * (vertices[0].y() - vertices[1].y()));
}

double distance(const Vec2& a, const Vec2& b) {
    return (a - b).norm();
}

Vec2 quadNaturalToGlobal(const Vec2& point, const std::vector<Vec2>& nodes) {
    if (nodes.size() != 4) {
        throw std::invalid_argument("quadNaturalToGlobal expects 4 nodes");
    }
    const double e = point.x();
    const double n = point.y();
    const double N1 = 0.25 * (1 - e) * (1 - n);
    const double N2 = 0.25 * (1 + e) * (1 - n);
    const double N3 = 0.25 * (1 + e) * (1 + n);
    const double N4 = 0.25 * (1 - e) * (1 + n);
    return N1 * nodes[0] + N2 * nodes[1] + N3 * nodes[2] + N4 * nodes[3];
}

Vec2 quadGlobalToNatural(const Vec2& point, const std::vector<Vec2>& nodes, int maxIterations) {
    Vec2 xi = Vec2::Zero();
    for (int iter = 0; iter < maxIterations; ++iter) {
        const double e = xi.x();
        const double n = xi.y();
        const double N1 = 0.25 * (1 - e) * (1 - n);
        const double N2 = 0.25 * (1 + e) * (1 - n);
        const double N3 = 0.25 * (1 + e) * (1 + n);
        const double N4 = 0.25 * (1 - e) * (1 + n);
        const Vec2 x = N1 * nodes[0] + N2 * nodes[1] + N3 * nodes[2] + N4 * nodes[3];
        const Vec2 residual = x - point;
        if (residual.norm() < 1e-12) {
            return xi;
        }
        Mat22 J;
        J << 0.25 * (-1 + n) * nodes[0].x() + 0.25 * (1 - n) * nodes[1].x() + 0.25 * (1 + n) * nodes[2].x() + 0.25 * (-1 - n) * nodes[3].x(),
             0.25 * (-1 + e) * nodes[0].x() + 0.25 * (-1 - e) * nodes[1].x() + 0.25 * (1 + e) * nodes[2].x() + 0.25 * (1 - e) * nodes[3].x(),
             0.25 * (-1 + n) * nodes[0].y() + 0.25 * (1 - n) * nodes[1].y() + 0.25 * (1 + n) * nodes[2].y() + 0.25 * (-1 - n) * nodes[3].y(),
             0.25 * (-1 + e) * nodes[0].y() + 0.25 * (-1 - e) * nodes[1].y() + 0.25 * (1 + e) * nodes[2].y() + 0.25 * (1 - e) * nodes[3].y();
        xi -= J.fullPivLu().solve(residual);
    }
    return xi;
}

std::vector<std::vector<Vec2>> subQuadMesh(const std::vector<Vec2>& nodes, int numberOfMesh) {
    std::vector<double> xs(numberOfMesh + 1), ys(numberOfMesh + 1);
    for (int i = 0; i <= numberOfMesh; ++i) {
        xs[i] = -1.0 + 2.0 * static_cast<double>(i) / static_cast<double>(numberOfMesh);
        ys[i] = -1.0 + 2.0 * static_cast<double>(i) / static_cast<double>(numberOfMesh);
    }

    std::vector<Vec2> points;
    for (double y : ys) {
        for (double x : xs) {
            points.emplace_back(x, y);
        }
    }

    std::vector<std::vector<Vec2>> result;
    const int width = numberOfMesh + 1;
    for (int j = 0; j < numberOfMesh; ++j) {
        for (int i = 0; i < numberOfMesh; ++i) {
            std::vector<int> domain = {
                i + j * width,
                i + 1 + j * width,
                i + 1 + (j + 1) * width,
                i + (j + 1) * width
            };
            std::vector<Vec2> sub(4);
            for (int k = 0; k < 4; ++k) {
                sub[k] = quadNaturalToGlobal(points[domain[k]], nodes);
            }
            result.push_back(sub);
        }
    }
    return result;
}

HESolveCrackPropagationQuadGaussData getQuadGaussData(int order) {
    HESolveCrackPropagationQuadGaussData data;
    std::vector<double> op;
    std::vector<double> wt;
    switch (order) {
        case 1:
            op = {0.0};
            wt = {2.0};
            break;
        case 2:
            op = {-1.0 / std::sqrt(3.0), 1.0 / std::sqrt(3.0)};
            wt = {1.0, 1.0};
            break;
        case 3:
            op = {-std::sqrt(3.0 / 5.0), 0.0, std::sqrt(3.0 / 5.0)};
            wt = {5.0 / 9.0, 8.0 / 9.0, 5.0 / 9.0};
            break;
        case 4: {
            const double a = std::sqrt(3.0 / 7.0 + (2.0 / 7.0) * std::sqrt(6.0 / 5.0));
            const double b = std::sqrt(3.0 / 7.0 - (2.0 / 7.0) * std::sqrt(6.0 / 5.0));
            op = {-a, -b, b, a};
            wt = {(18.0 - std::sqrt(30.0)) / 36.0, (18.0 + std::sqrt(30.0)) / 36.0,
                  (18.0 + std::sqrt(30.0)) / 36.0, (18.0 - std::sqrt(30.0)) / 36.0};
            break;
        }
        case 5: {
            const double a = (1.0 / 3.0) * std::sqrt(5.0 + 2.0 * std::sqrt(10.0 / 7.0));
            const double b = (1.0 / 3.0) * std::sqrt(5.0 - 2.0 * std::sqrt(10.0 / 7.0));
            op = {-a, -b, 0.0, b, a};
            wt = {(322.0 - 13.0 * std::sqrt(70.0)) / 900.0, (322.0 + 13.0 * std::sqrt(70.0)) / 900.0,
                  128.0 / 225.0, (322.0 + 13.0 * std::sqrt(70.0)) / 900.0, (322.0 - 13.0 * std::sqrt(70.0)) / 900.0};
            break;
        }
        default:
            throw std::invalid_argument("Unsupported quadrilateral Gauss order");
    }

    for (double x : op) {
        for (double y : op) {
            data.points.emplace_back(x, y);
        }
    }
    for (double wx : wt) {
        for (double wy : wt) {
            data.weights.push_back(wx * wy);
        }
    }
    return data;
}

HESolveCrackPropagationQuadGaussData getQuadGaussPoint(int numberOfMesh, int order) {
    const std::vector<Vec2> ref = {Vec2(-1, -1), Vec2(1, -1), Vec2(1, 1), Vec2(-1, 1)};
    auto subNodes = subQuadMesh(ref, numberOfMesh);
    HESolveCrackPropagationQuadGaussData result;
    for (const auto& sub : subNodes) {
        const auto local = getQuadGaussData(order);
        for (std::size_t i = 0; i < local.points.size(); ++i) {
            result.points.push_back(quadNaturalToGlobal(local.points[i], sub));
            result.weights.push_back(local.weights[i] / static_cast<double>(numberOfMesh * numberOfMesh));
        }
    }
    return result;
}

HESolveCrackPropagationTriGaussData getTriGaussData(int order) {
    HESolveCrackPropagationTriGaussData data;
    switch (order) {
        case 1:
            data.points = {Vec2(1.0 / 3.0, 1.0 / 3.0)};
            data.weights = {1.0};
            break;
        case 2:
            data.points = {Vec2(1.0 / 6.0, 1.0 / 6.0), Vec2(2.0 / 3.0, 1.0 / 6.0), Vec2(1.0 / 6.0, 2.0 / 3.0)};
            data.weights = {1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0};
            break;
        case 3:
            data.points = {Vec2(1.0 / 3.0, 1.0 / 3.0), Vec2(1.0 / 5.0, 3.0 / 5.0), Vec2(1.0 / 5.0, 1.0 / 5.0), Vec2(3.0 / 5.0, 1.0 / 5.0)};
            data.weights = {-27.0 / 48.0, 25.0 / 48.0, 25.0 / 48.0, 25.0 / 48.0};
            break;
        default:
            throw std::invalid_argument("Unsupported triangle Gauss order");
    }
    return data;
}

HESolveCrackPropagationTriGaussData getTriGaussPoint(const std::vector<Vec2>& nodes, int order) {
    HESolveCrackPropagationTriGaussData out;
    const auto base = getTriGaussData(order);
    const double area = triArea(nodes);
    out.weights.reserve(base.weights.size());
    out.points.reserve(base.points.size());
    for (std::size_t i = 0; i < base.points.size(); ++i) {
        out.points.push_back(triNaturalToGlobal(base.points[i], nodes));
        out.weights.push_back(area * base.weights[i]);
    }
    return out;
}

Vec2 triNaturalToGlobal(const Vec2& point, const std::vector<Vec2>& nodes) {
    const double e = point.x();
    const double n = point.y();
    const double N1 = 1.0 - e - n;
    const double N2 = e;
    const double N3 = n;
    return N1 * nodes[0] + N2 * nodes[1] + N3 * nodes[2];
}

double getLevelSet(const std::vector<Vec2>& crack, const Vec2& point) {
    const double length = 10.0 * distance(crack.front(), crack.back());
    std::vector<Vec2> polygon = crack;
    polygon.push_back(Vec2(crack.back().x() + length, crack.back().y()));
    polygon.push_back(Vec2(crack.back().x() + length, crack.back().y() - length));
    polygon.push_back(Vec2(crack.front().x() - length, crack.front().y() - length));
    polygon.push_back(Vec2(crack.front().x() - length, crack.front().y()));
    return pointInPolygon(point, polygon) ? -1.0 : 1.0;
}

bool isItIntersect(const std::vector<Vec2>& line, const std::vector<Vec2>& polygon) {
    if (line.size() < 2 || polygon.size() < 3) {
        return false;
    }
    for (std::size_t i = 0; i + 1 < line.size(); ++i) {
        for (std::size_t j = 0; j < polygon.size(); ++j) {
            const auto& a = polygon[j];
            const auto& b = polygon[(j + 1) % polygon.size()];
            if (segmentIntersection(line[i], line[i + 1], a, b).has_value()) {
                return true;
            }
        }
    }
    return false;
}

std::vector<Vec2> getIntersectionPoints(const std::vector<Vec2>& line, const std::vector<Vec2>& polygon) {
    std::vector<Vec2> result;
    for (std::size_t i = 0; i + 1 < line.size(); ++i) {
        for (std::size_t j = 0; j < polygon.size(); ++j) {
            const auto& a = polygon[j];
            const auto& b = polygon[(j + 1) % polygon.size()];
            if (auto p = segmentIntersection(line[i], line[i + 1], a, b)) {
                uniquePush(result, *p);
            }
        }
    }
    return result;
}

std::vector<Vec2> getPolygonOrdinary(const std::vector<Vec2>& firstSetPoints, const std::vector<Vec2>& secondSetPoints) {
    if (firstSetPoints.size() == 1) {
        std::vector<Vec2> out = firstSetPoints;
        out.insert(out.end(), secondSetPoints.begin(), secondSetPoints.end());
        return out;
    }
    std::vector<Vec2> first = firstSetPoints;
    first.insert(first.end(), secondSetPoints.begin(), secondSetPoints.end());

    std::vector<Vec2> rev = firstSetPoints;
    std::reverse(rev.begin(), rev.end());
    rev.insert(rev.end(), secondSetPoints.begin(), secondSetPoints.end());

    const double a1 = std::abs(polygonSignedArea(first));
    const double a2 = std::abs(polygonSignedArea(rev));
    return (a2 < a1) ? first : rev;
}

HESolveCrackPropagationTriangleMesh getTriMesh(const std::vector<Vec2>& polygon) {
    HESolveCrackPropagationTriangleMesh mesh;
    mesh.points = polygon;
    if (polygon.size() < 3) {
        return mesh;
    }

    std::vector<int> idx(polygon.size());
    std::iota(idx.begin(), idx.end(), 0);
    const bool ccw = polygonSignedArea(polygon) > 0.0;

    int guard = 0;
    while (idx.size() > 3 && guard++ < 1000) {
        bool earFound = false;
        for (std::size_t i = 0; i < idx.size(); ++i) {
            const int prev = idx[(i + idx.size() - 1) % idx.size()];
            const int curr = idx[i];
            const int next = idx[(i + 1) % idx.size()];
            const Vec2& a = polygon[prev];
            const Vec2& b = polygon[curr];
            const Vec2& c = polygon[next];
            if (!isConvex(a, b, c, ccw)) {
                continue;
            }
            bool contains = false;
            for (int other : idx) {
                if (other == prev || other == curr || other == next) {
                    continue;
                }
                if (pointInTriangle(polygon[other], a, b, c)) {
                    contains = true;
                    break;
                }
            }
            if (!contains) {
                mesh.triangles.emplace_back(prev, curr, next);
                idx.erase(idx.begin() + static_cast<long>(i));
                earFound = true;
                break;
            }
        }
        if (!earFound) {
            break;
        }
    }
    if (idx.size() == 3) {
        mesh.triangles.emplace_back(idx[0], idx[1], idx[2]);
    }
    return mesh;
}

int getEdgeContainPoint(const Vec2& point, const std::vector<Vec2>& polygon) {
    double best = std::numeric_limits<double>::max();
    int bestIdx = 0;
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        const double d = pointLineDistance(point, polygon[(i + polygon.size() - 1) % polygon.size()], polygon[i]);
        if (d < best) {
            best = d;
            bestIdx = static_cast<int>(i);
        }
    }
    return bestIdx;
}

std::vector<Vec2> getCrackInPolygon(const std::vector<Vec2>& crack, const std::vector<Vec2>& polygon) {
    std::vector<Vec2> inside;
    if (!isItIntersect(crack, polygon)) {
        return inside;
    }
    bool reverse = false;
    for (std::size_t i = 0; i + 1 < crack.size(); ++i) {
        if (pointInPolygon(crack[i], polygon)) {
            uniquePush(inside, crack[i]);
            if (i == 0) {
                reverse = true;
            }
        }
        auto intersections = getIntersectionPoints({crack[i], crack[i + 1]}, polygon);
        for (const auto& p : intersections) {
            uniquePush(inside, p);
        }
    }
    if (pointInPolygon(crack.back(), polygon)) {
        uniquePush(inside, crack.back());
    }
    if (reverse) {
        std::reverse(inside.begin(), inside.end());
    }
    return inside;
}

} // namespace XFEMCrackPropagation
