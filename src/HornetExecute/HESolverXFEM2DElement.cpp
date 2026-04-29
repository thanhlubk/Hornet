#include "HornetExecute/HESolverXFEM2DElement.h"

namespace xfem {
namespace {


int wrap4(int i) {
    int v = i % 4;
    return v < 0 ? v + 4 : v;
}

std::vector<Vec2> extractCoordinates(const std::vector<HESolverXFEM2DNodePtr>& nodes) {
    std::vector<Vec2> out;
    out.reserve(nodes.size());
    for (const auto& node : nodes) {
        out.push_back(node->coordinate());
    }
    return out;
}

Eigen::Matrix<double, 2, 2> rotation(double alpha) {
    Eigen::Matrix<double, 2, 2> QT;
    QT << std::cos(alpha), std::sin(alpha),
         -std::sin(alpha), std::cos(alpha);
    return QT;
}

void accumulateNodeStress(const Mat33& dMatrix, const Eigen::MatrixXd& bMatrix, const Eigen::VectorXd& displacement, HESolverXFEM2DNodePtr node) {
    const Vec3 sigma = dMatrix * (bMatrix * displacement);
    node->stressList().push_back(HESolverXFEM2DElement::stressWithVonMises(sigma));
}

} // namespace

HESolverXFEM2DElement::HESolverXFEM2DElement(std::vector<HESolverXFEM2DNodePtr> nodes, const Mat33& dMatrix, double thickness, double density)
    : nodes_(std::move(nodes)),
    dMatrix_(dMatrix),
    thickness_(thickness),
    density_(density) {}

void HESolverXFEM2DElement::createDisplacement(const Eigen::VectorXd& displacementOfElement) {
    displacement_ = displacementOfElement;
}

void HESolverXFEM2DElement::appendColumns(Eigen::MatrixXd& dst, const Eigen::MatrixXd& src) {
    const Eigen::Index oldCols = dst.cols();
    dst.conservativeResize(dst.rows(), oldCols + src.cols());
    dst.block(0, oldCols, dst.rows(), src.cols()) = src;
}

Mat22 HESolverXFEM2DElement::jacobianForOneGaussPoint(const Vec2& gpts) const {
    const double e = gpts.x();
    const double n = gpts.y();
    Eigen::Matrix<double, 2, 4> dN;
    dN << -0.25 * (1 - n),  0.25 * (1 - n),  0.25 * (1 + n), -0.25 * (1 + n),
          -0.25 * (1 - e), -0.25 * (1 + e),  0.25 * (1 + e),  0.25 * (1 - e);

    Eigen::Matrix<double, 4, 2> xy;
    for (int i = 0; i < 4; ++i) {
        xy(i, 0) = nodes_[i]->coordinate().x();
        xy(i, 1) = nodes_[i]->coordinate().y();
    }
    return dN * xy;
}

Mat28 HESolverXFEM2DElement::shapeMatrixForOneGaussPoint(const Vec2& gpts) {
    const double e = gpts.x();
    const double n = gpts.y();
    const std::array<double, 4> shape = {
        0.25 * (1 - e) * (1 - n),
        0.25 * (1 + e) * (1 - n),
        0.25 * (1 + e) * (1 + n),
        0.25 * (1 - e) * (1 + n)
    };

    Mat28 out = Mat28::Zero();
    for (int i = 0; i < 4; ++i) {
        out(0, i * 2) = shape[i];
        out(1, i * 2 + 1) = shape[i];
    }
    return out;
}

Eigen::MatrixXd HESolverXFEM2DElement::bMatrixForOneGaussPoint(const Vec2& gpts) const {
    const auto J = jacobianForOneGaussPoint(gpts);
    const double detJ = J.determinant();
    Mat34 A;
    A <<  J(1, 1), -J(0, 1),        0.0,        0.0,
                 0.0,        0.0, -J(1, 0),  J(0, 0),
           -J(1, 0),  J(0, 0),  J(1, 1), -J(0, 1);
    A *= (1.0 / detJ);

    const double e = gpts.x();
    const double n = gpts.y();
    Eigen::Matrix<double, 4, 8> G;
    G << n - 1, 0, 1 - n, 0, 1 + n, 0, -1 - n, 0,
         e - 1, 0, -1 - e, 0, 1 + e, 0, 1 - e, 0,
         0, n - 1, 0, 1 - n, 0, 1 + n, 0, -1 - n,
         0, e - 1, 0, -1 - e, 0, 1 + e, 0, 1 - e;
    G *= 0.25;

    return A * G;
}

Vec4 HESolverXFEM2DElement::stressWithVonMises(const Vec3& sigma) {
    const double vm = std::sqrt(((sigma(0) - sigma(1)) * (sigma(0) - sigma(1)) +
                                 sigma(0) * sigma(0) + sigma(1) * sigma(1) +
                                 6.0 * sigma(2) * sigma(2)) / 2.0);
    return Vec4(sigma(0), sigma(1), sigma(2), vm);
}

void HESolverXFEM2DElement::createGlobalGaussPoint() {
    globalGaussPoints_.clear();
    const auto coords = extractCoordinates(nodes_);
    for (const auto& gp : gaussPoints_) {
        globalGaussPoints_.push_back(quadNaturalToGlobal(gp, coords));
    }
}

void HESolverXFEM2DElement::createStress() {
    createGlobalGaussPoint();
    stress_.clear();
    for (std::size_t i = 0; i < gaussPoints_.size(); ++i) {
        const Vec3 sigma = dMatrix_ * (bMatrixForAllGaussPoint_[i] * displacement_);
        stress_.push_back(stressWithVonMises(sigma));
    }
    const std::vector<Vec2> nodeGaussPoint = {Vec2(-1, -1), Vec2(1, -1), Vec2(1, 1), Vec2(-1, 1)};
    for (int i = 0; i < 4; ++i) {
        accumulateNodeStress(dMatrix_, bMatrixForOneGaussPoint(nodeGaussPoint[i]), displacement_, nodes_[i]);
    }
}

void HESolverXFEM2DStandardElement::createGaussPoint() {
    const auto data = getQuadGaussPoint(1, 2);
    gaussPoints_ = data.points;
    gaussWeight_ = data.weights;
}

void HESolverXFEM2DStandardElement::createStiffnessMatrix() {
    stiffnessMatrix_ = Eigen::MatrixXd::Zero(localDofs(), localDofs());
    bMatrixForAllGaussPoint_.clear();
    for (std::size_t i = 0; i < gaussPoints_.size(); ++i) {
        const auto B = bMatrixForOneGaussPoint(gaussPoints_[i]);
        bMatrixForAllGaussPoint_.push_back(B);
        const auto J = jacobianForOneGaussPoint(gaussPoints_[i]);
        stiffnessMatrix_ += thickness_ * J.determinant() * gaussWeight_[i] * (B.transpose() * dMatrix_ * B);
    }
}

void HESolverXFEM2DStandardElement::createMassMatrix() {
    massMatrix_ = Eigen::MatrixXd::Zero(localDofs(), localDofs());
    for (std::size_t i = 0; i < gaussPoints_.size(); ++i) {
        const auto N = shapeMatrixForOneGaussPoint(gaussPoints_[i]);
        const auto J = jacobianForOneGaussPoint(gaussPoints_[i]);
        massMatrix_ += thickness_ * density_ * J.determinant() * gaussWeight_[i] * (N.transpose() * N);
    }
}

HESolverXFEM2DCrackBodyElement::HESolverXFEM2DCrackBodyElement(std::vector<HESolverXFEM2DNodePtr> nodes, const Mat33& dMatrix, std::vector<Vec2> crack, double thickness, double density)
    : HESolverXFEM2DElement(std::move(nodes), dMatrix, thickness, density), crack_(std::move(crack)) {
    classify_ = "CrackBody";
}

void HESolverXFEM2DCrackBodyElement::createGaussPoint() {
    gaussPoints_.clear();
    gaussWeight_.clear();
    gaussLevelSet_.clear();

    std::vector<int> low;
    std::vector<int> high;
    for (int i = 0; i < 4; ++i) {
        if (nodes_[i]->levelSet() == -1) {
            low.push_back(i);
        } else {
            high.push_back(i);
        }
    }

    if (low.size() == 2) {
        firstSubPolygonGlb_ = getPolygonOrdinary({nodes_[low[0]]->coordinate(), nodes_[low[1]]->coordinate()}, crack_);
        secondSubPolygonGlb_ = getPolygonOrdinary({nodes_[high[0]]->coordinate(), nodes_[high[1]]->coordinate()}, crack_);
    } else if (low.size() == 1) {
        firstSubPolygonGlb_ = {nodes_[low[0]]->coordinate()};
        firstSubPolygonGlb_.insert(firstSubPolygonGlb_.end(), crack_.begin(), crack_.end());
        secondSubPolygonGlb_ = {nodes_[high[0]]->coordinate(), nodes_[high[1]]->coordinate(), nodes_[high[2]]->coordinate()};
        secondSubPolygonGlb_ = getPolygonOrdinary(secondSubPolygonGlb_, crack_);
    } else if (low.size() == 3) {
        firstSubPolygonGlb_ = {nodes_[low[0]]->coordinate(), nodes_[low[1]]->coordinate(), nodes_[low[2]]->coordinate()};
        firstSubPolygonGlb_ = getPolygonOrdinary(firstSubPolygonGlb_, crack_);
        secondSubPolygonGlb_ = {nodes_[high[0]]->coordinate()};
        secondSubPolygonGlb_.insert(secondSubPolygonGlb_.end(), crack_.begin(), crack_.end());
    }

    firstSubPolygon_.clear();
    secondSubPolygon_.clear();
    const auto coords = extractCoordinates(nodes_);
    for (const auto& p : firstSubPolygonGlb_) {
        firstSubPolygon_.push_back(quadGlobalToNatural(p, coords));
    }
    for (const auto& p : secondSubPolygonGlb_) {
        secondSubPolygon_.push_back(quadGlobalToNatural(p, coords));
    }

    const auto mesh1 = getTriMesh(firstSubPolygon_);
    const auto mesh2 = getTriMesh(secondSubPolygon_);
    for (const auto& tri : mesh1.triangles) {
        std::vector<Vec2> triPts = {mesh1.points[tri[0]], mesh1.points[tri[1]], mesh1.points[tri[2]]};
        const auto g = getTriGaussPoint(triPts, 2);
        for (std::size_t i = 0; i < g.points.size(); ++i) {
            gaussPoints_.push_back(g.points[i]);
            gaussWeight_.push_back(g.weights[i]);
            gaussLevelSet_.push_back(-1.0);
        }
    }
    for (const auto& tri : mesh2.triangles) {
        std::vector<Vec2> triPts = {mesh2.points[tri[0]], mesh2.points[tri[1]], mesh2.points[tri[2]]};
        const auto g = getTriGaussPoint(triPts, 2);
        for (std::size_t i = 0; i < g.points.size(); ++i) {
            gaussPoints_.push_back(g.points[i]);
            gaussWeight_.push_back(g.weights[i]);
            gaussLevelSet_.push_back(1.0);
        }
    }
}

Eigen::MatrixXd HESolverXFEM2DCrackBodyElement::bMatrixForOneGaussPoint(const Vec2& gpts) const {
    const double ls = gaussLevelSet_.empty() ? 0.0 : gaussLevelSet_.front();
    return bMatrixForOneGaussPoint(gpts, ls);
}

Eigen::MatrixXd HESolverXFEM2DCrackBodyElement::bMatrixForOneGaussPoint(const Vec2& gpts, double gptsLevelSet) const {
    const auto stdB = HESolverXFEM2DElement::bMatrixForOneGaussPoint(gpts);
    Eigen::MatrixXd out(3, 0);
    for (int i = 0; i < 4; ++i) {
        Eigen::Matrix<double, 3, 2> sub;
        const double first = stdB(0, i * 2);
        const double second = stdB(1, i * 2 + 1);
        const double bLevel = gptsLevelSet - nodes_[i]->levelSet();
        sub << first, 0.0,
               0.0, second,
               second, first;
        sub *= bLevel;
        appendColumns(out, sub);
    }
    Eigen::MatrixXd full = stdB;
    appendColumns(full, out);
    return full;
}

Eigen::MatrixXd HESolverXFEM2DCrackBodyElement::shapeMatrixForOneGaussPoint(const Vec2& gpts, double gptsLevelSet) const {
    const auto stdN = HESolverXFEM2DElement::shapeMatrixForOneGaussPoint(gpts);
    Eigen::MatrixXd enr(2, 0);
    for (int i = 0; i < 4; ++i) {
        Eigen::Matrix<double, 2, 2> sub = Eigen::Matrix<double, 2, 2>::Zero();
        const double shape = stdN(0, i * 2);
        const double level = gptsLevelSet - nodes_[i]->levelSet();
        sub(0, 0) = shape * level;
        sub(1, 1) = shape * level;
        appendColumns(enr, sub);
    }
    Eigen::MatrixXd full = stdN;
    appendColumns(full, enr);
    return full;
}

void HESolverXFEM2DCrackBodyElement::createStiffnessMatrix() {
    stiffnessMatrix_ = Eigen::MatrixXd::Zero(localDofs(), localDofs());
    bMatrixForAllGaussPoint_.clear();
    for (std::size_t i = 0; i < gaussPoints_.size(); ++i) {
        const auto B = bMatrixForOneGaussPoint(gaussPoints_[i], gaussLevelSet_[i]);
        bMatrixForAllGaussPoint_.push_back(B);
        const auto J = jacobianForOneGaussPoint(gaussPoints_[i]);
        stiffnessMatrix_ += thickness_ * J.determinant() * gaussWeight_[i] * (B.transpose() * dMatrix_ * B);
    }
}

void HESolverXFEM2DCrackBodyElement::createMassMatrix() {
    massMatrix_ = Eigen::MatrixXd::Zero(localDofs(), localDofs());
    for (std::size_t i = 0; i < gaussPoints_.size(); ++i) {
        const auto N = shapeMatrixForOneGaussPoint(gaussPoints_[i], gaussLevelSet_[i]);
        const auto J = jacobianForOneGaussPoint(gaussPoints_[i]);
        massMatrix_ += thickness_ * density_ * J.determinant() * gaussWeight_[i] * (N.transpose() * N);
    }
}

void HESolverXFEM2DCrackBodyElement::createDisplacement(const Eigen::VectorXd& displacementOfElement) {
    displacement_ = displacementOfElement;
    firstSubPolygonDisplacement_.clear();
    secondSubPolygonDisplacement_.clear();
    for (const auto& p : firstSubPolygon_) {
        const auto v = shapeMatrixForOneGaussPoint(p, -1.0) * displacementOfElement;
        firstSubPolygonDisplacement_.push_back(v);
    }
    for (const auto& p : secondSubPolygon_) {
        const auto v = shapeMatrixForOneGaussPoint(p, 1.0) * displacementOfElement;
        secondSubPolygonDisplacement_.push_back(v);
    }
}

void HESolverXFEM2DCrackBodyElement::createStress() {
    createGlobalGaussPoint();
    stress_.clear();
    for (std::size_t i = 0; i < gaussPoints_.size(); ++i) {
        const Vec3 sigma = dMatrix_ * (bMatrixForAllGaussPoint_[i] * displacement_);
        stress_.push_back(stressWithVonMises(sigma));
    }
    const std::vector<Vec2> nodeGaussPoint = {Vec2(-1, -1), Vec2(1, -1), Vec2(1, 1), Vec2(-1, 1)};
    for (int i = 0; i < 4; ++i) {
        accumulateNodeStress(dMatrix_, bMatrixForOneGaussPoint(nodeGaussPoint[i], nodes_[i]->levelSet()), displacement_, nodes_[i]);
    }
}

HESolverXFEM2DCrackTipElement::HESolverXFEM2DCrackTipElement(std::vector<HESolverXFEM2DNodePtr> nodes, const Mat33& dMatrix, std::vector<Vec2> crack, double thickness, double density)
    : HESolverXFEM2DElement(std::move(nodes), dMatrix, thickness, density), crack_(std::move(crack)),
      alpha_(std::atan2(crack_.back().y() - crack_[crack_.size() - 2].y(), crack_.back().x() - crack_[crack_.size() - 2].x())) {
    classify_ = "CrackTip";
    for (auto& node : nodes_) {
        node->setNearestTip(crack_.back());
        node->setTipAngle(alpha_);
    }
}

Eigen::Vector4d HESolverXFEM2DCrackTipElement::branch(double r, double theta) {
    const double r2 = safeSqrt(r);
    const double st2 = std::sin(theta / 2.0);
    const double ct2 = std::cos(theta / 2.0);
    const double st = std::sin(theta);
    return Eigen::Vector4d(r2 * st2, r2 * ct2, r2 * st2 * st, r2 * ct2 * st);
}

std::array<Eigen::Vector4d, 3> HESolverXFEM2DCrackTipElement::branchDerivative(double r, double theta, double alpha) {
    const double r2 = safeSqrt(r);
    const double fac = 0.5 / std::max(r2, 1e-12);
    const double st2 = std::sin(theta / 2.0);
    const double ct2 = std::cos(theta / 2.0);
    const double s3t2 = std::sin(1.5 * theta);
    const double c3t2 = std::cos(1.5 * theta);
    const double st = std::sin(theta);
    const double ct = std::cos(theta);
    Eigen::Vector4d f = branch(r, theta);
    Eigen::Vector4d dPhiDx1, dPhiDx2;
    dPhiDx1 << -fac * st2, fac * ct2, -fac * s3t2 * st, -fac * c3t2 * st;
    dPhiDx2 << fac * ct2, fac * st2, fac * (st2 + s3t2 * ct), fac * (ct2 + c3t2 * ct);

    const double dx1dx = std::cos(alpha);
    const double dx2dx = -std::sin(alpha);
    const double dx1dy = std::sin(alpha);
    const double dx2dy = std::cos(alpha);
    Eigen::Vector4d dfdx = dPhiDx1 * dx1dx + dPhiDx2 * dx2dx;
    Eigen::Vector4d dfdy = dPhiDx1 * dx1dy + dPhiDx2 * dx2dy;
    return {f, dfdx, dfdy};
}

Eigen::MatrixXd HESolverXFEM2DCrackTipElement::bMatrixForOneGaussPoint(const Vec2& gpts) const {
    const auto stdB = HESolverXFEM2DElement::bMatrixForOneGaussPoint(gpts);
    const auto stdN = HESolverXFEM2DElement::shapeMatrixForOneGaussPoint(gpts);
    const auto QT = rotation(alpha_);
    const Vec2 xy = quadNaturalToGlobal(gpts, extractCoordinates(nodes_));
    const Vec2 xp = QT * (xy - crack_.back());
    const double r = xp.norm();
    const double theta = std::atan2(xp.y(), xp.x());
    const auto deriv = branchDerivative(r, theta, alpha_);
    const auto& Br = deriv[0];
    const auto& dBdx = deriv[1];
    const auto& dBdy = deriv[2];

    Eigen::MatrixXd enr(3, 0);
    for (int i = 0; i < 4; ++i) {
        const Vec2 xpi = QT * (nodes_[i]->coordinate() - crack_.back());
        const double ri = xpi.norm();
        const double thetai = std::atan2(xpi.y(), xpi.x());
        const auto BrI = branch(ri, thetai);

        for (int mode = 0; mode < 4; ++mode) {
            Eigen::Matrix<double, 3, 2> sub;
            const double first = stdB(0, i * 2) * (Br(mode) - BrI(mode)) + stdN(0, i * 2) * dBdx(mode);
            const double second = stdB(1, i * 2 + 1) * (Br(mode) - BrI(mode)) + stdN(1, i * 2 + 1) * dBdy(mode);
            sub << first, 0.0,
                   0.0, second,
                   second, first;
            appendColumns(enr, sub);
        }
    }
    Eigen::MatrixXd full = stdB;
    appendColumns(full, enr);
    return full;
}

Eigen::MatrixXd HESolverXFEM2DCrackTipElement::shapeMatrixForOneGaussPoint(const Vec2& gpts) const {
    const auto stdN = HESolverXFEM2DElement::shapeMatrixForOneGaussPoint(gpts);
    const auto QT = rotation(alpha_);
    const Vec2 xy = quadNaturalToGlobal(gpts, extractCoordinates(nodes_));
    const Vec2 xp = QT * (xy - crack_.back());
    const auto Br = branch(xp.norm(), std::atan2(xp.y(), xp.x()));

    Eigen::MatrixXd enr(2, 0);
    for (int i = 0; i < 4; ++i) {
        const Vec2 xpi = QT * (nodes_[i]->coordinate() - crack_.back());
        const auto BrI = branch(xpi.norm(), std::atan2(xpi.y(), xpi.x()));
        for (int mode = 0; mode < 4; ++mode) {
            Eigen::Matrix<double, 2, 2> sub = Eigen::Matrix<double, 2, 2>::Zero();
            const double value = stdN(0, i * 2) * (Br(mode) - BrI(mode));
            sub(0, 0) = value;
            sub(1, 1) = value;
            appendColumns(enr, sub);
        }
    }
    Eigen::MatrixXd full = stdN;
    appendColumns(full, enr);
    return full;
}

void HESolverXFEM2DCrackTipElement::createGaussPoint() {
    gaussPoints_.clear();
    gaussWeight_.clear();
    const int direction = getEdgeContainPoint(crack_.front(), extractCoordinates(nodes_));
    std::vector<Vec2> firstGlb = crack_;
    firstGlb.push_back(nodes_[wrap4(direction - 3)]->coordinate());
    firstGlb.push_back(nodes_[wrap4(direction - 4)]->coordinate());
    std::vector<Vec2> secondGlb = crack_;
    secondGlb.push_back(nodes_[wrap4(direction - 3)]->coordinate());
    secondGlb.push_back(nodes_[wrap4(direction - 2)]->coordinate());
    secondGlb.push_back(nodes_[wrap4(direction - 1)]->coordinate());

    std::vector<Vec2> firstNat, secondNat;
    const auto coords = extractCoordinates(nodes_);
    for (const auto& p : firstGlb) {
        firstNat.push_back(quadGlobalToNatural(p, coords));
    }
    for (const auto& p : secondGlb) {
        secondNat.push_back(quadGlobalToNatural(p, coords));
    }

    const auto mesh1 = getTriMesh(firstNat);
    const auto mesh2 = getTriMesh(secondNat);
    for (const auto& tri : mesh1.triangles) {
        std::vector<Vec2> triPts = {mesh1.points[tri[0]], mesh1.points[tri[1]], mesh1.points[tri[2]]};
        const auto g = getTriGaussPoint(triPts, 3);
        gaussPoints_.insert(gaussPoints_.end(), g.points.begin(), g.points.end());
        gaussWeight_.insert(gaussWeight_.end(), g.weights.begin(), g.weights.end());
    }
    for (const auto& tri : mesh2.triangles) {
        std::vector<Vec2> triPts = {mesh2.points[tri[0]], mesh2.points[tri[1]], mesh2.points[tri[2]]};
        const auto g = getTriGaussPoint(triPts, 3);
        gaussPoints_.insert(gaussPoints_.end(), g.points.begin(), g.points.end());
        gaussWeight_.insert(gaussWeight_.end(), g.weights.begin(), g.weights.end());
    }
}

void HESolverXFEM2DCrackTipElement::createStiffnessMatrix() {
    stiffnessMatrix_ = Eigen::MatrixXd::Zero(localDofs(), localDofs());
    bMatrixForAllGaussPoint_.clear();
    for (std::size_t i = 0; i < gaussPoints_.size(); ++i) {
        const auto B = bMatrixForOneGaussPoint(gaussPoints_[i]);
        bMatrixForAllGaussPoint_.push_back(B);
        const auto J = jacobianForOneGaussPoint(gaussPoints_[i]);
        stiffnessMatrix_ += thickness_ * J.determinant() * gaussWeight_[i] * (B.transpose() * dMatrix_ * B);
    }
}

void HESolverXFEM2DCrackTipElement::createMassMatrix() {
    massMatrix_ = Eigen::MatrixXd::Zero(localDofs(), localDofs());
    for (std::size_t i = 0; i < gaussPoints_.size(); ++i) {
        const auto N = shapeMatrixForOneGaussPoint(gaussPoints_[i]);
        const auto J = jacobianForOneGaussPoint(gaussPoints_[i]);
        massMatrix_ += thickness_ * density_ * J.determinant() * gaussWeight_[i] * (N.transpose() * N);
    }
}

HESolverXFEM2DBlendedElement::HESolverXFEM2DBlendedElement(std::vector<HESolverXFEM2DNodePtr> nodes, const Mat33& dMatrix, std::vector<Vec2> crack, double thickness, double density)
    : HESolverXFEM2DElement(std::move(nodes), dMatrix, thickness, density), crack_(std::move(crack)) {
    classify_ = "Blended";
    dofs_ = 8;
    for (const auto& node : nodes_) {
        if (node->classify() == HESolverXFEM2DNodeClass::Heaviside) {
            dofs_ += 2;
        } else if (node->classify() == HESolverXFEM2DNodeClass::Asymptotic) {
            dofs_ += 8;
        }
    }
}

Eigen::Vector4d HESolverXFEM2DBlendedElement::branch(double r, double theta) {
    return HESolverXFEM2DCrackTipElement::branch(r, theta);
}

std::array<Eigen::Vector4d, 3> HESolverXFEM2DBlendedElement::branchDerivative(double r, double theta, double alpha) {
    return HESolverXFEM2DCrackTipElement::branchDerivative(r, theta, alpha);
}

void HESolverXFEM2DBlendedElement::createGaussPoint() {
    gaussPoints_.clear();
    gaussWeight_.clear();
    gaussLevelSet_.clear();

    if (crack_.empty()) {
        const bool hasAsym = std::any_of(nodes_.begin(), nodes_.end(), [](const HESolverXFEM2DNodePtr& node) { return node->classify() == HESolverXFEM2DNodeClass::Asymptotic; });
        const auto data = hasAsym ? getQuadGaussPoint(2, 4) : getQuadGaussPoint(1, 2);
        gaussPoints_ = data.points;
        gaussWeight_ = data.weights;

        const auto it = std::find_if(nodes_.begin(), nodes_.end(), [](const HESolverXFEM2DNodePtr& node) { return node->classify() == HESolverXFEM2DNodeClass::Heaviside; });
        if (it != nodes_.end()) {
            gaussLevelSet_.assign(gaussPoints_.size(), (*it)->levelSet());
        } else {
            gaussLevelSet_.assign(gaussPoints_.size(), 0.0);
        }
        return;
    }

    std::vector<int> low;
    std::vector<int> high;
    for (int i = 0; i < 4; ++i) {
        if (nodes_[i]->levelSet() == -1) {
            low.push_back(i);
        } else {
            high.push_back(i);
        }
    }

    if (low.size() == 2) {
        firstSubPolygonGlb_ = getPolygonOrdinary({nodes_[low[0]]->coordinate(), nodes_[low[1]]->coordinate()}, crack_);
        secondSubPolygonGlb_ = getPolygonOrdinary({nodes_[high[0]]->coordinate(), nodes_[high[1]]->coordinate()}, crack_);
    } else if (low.size() == 1) {
        firstSubPolygonGlb_ = {nodes_[low[0]]->coordinate()};
        firstSubPolygonGlb_.insert(firstSubPolygonGlb_.end(), crack_.begin(), crack_.end());
        secondSubPolygonGlb_ = {nodes_[high[0]]->coordinate(), nodes_[high[1]]->coordinate(), nodes_[high[2]]->coordinate()};
        secondSubPolygonGlb_ = getPolygonOrdinary(secondSubPolygonGlb_, crack_);
    } else if (low.size() == 3) {
        firstSubPolygonGlb_ = {nodes_[low[0]]->coordinate(), nodes_[low[1]]->coordinate(), nodes_[low[2]]->coordinate()};
        firstSubPolygonGlb_ = getPolygonOrdinary(firstSubPolygonGlb_, crack_);
        secondSubPolygonGlb_ = {nodes_[high[0]]->coordinate()};
        secondSubPolygonGlb_.insert(secondSubPolygonGlb_.end(), crack_.begin(), crack_.end());
    }

    firstSubPolygon_.clear();
    secondSubPolygon_.clear();
    const auto coords = extractCoordinates(nodes_);
    for (const auto& p : firstSubPolygonGlb_) {
        firstSubPolygon_.push_back(quadGlobalToNatural(p, coords));
    }
    for (const auto& p : secondSubPolygonGlb_) {
        secondSubPolygon_.push_back(quadGlobalToNatural(p, coords));
    }

    const auto mesh1 = getTriMesh(firstSubPolygon_);
    const auto mesh2 = getTriMesh(secondSubPolygon_);
    for (const auto& tri : mesh1.triangles) {
        std::vector<Vec2> triPts = {mesh1.points[tri[0]], mesh1.points[tri[1]], mesh1.points[tri[2]]};
        const auto g = getTriGaussPoint(triPts, 3);
        for (std::size_t i = 0; i < g.points.size(); ++i) {
            gaussPoints_.push_back(g.points[i]);
            gaussWeight_.push_back(g.weights[i]);
            gaussLevelSet_.push_back(-1.0);
        }
    }
    for (const auto& tri : mesh2.triangles) {
        std::vector<Vec2> triPts = {mesh2.points[tri[0]], mesh2.points[tri[1]], mesh2.points[tri[2]]};
        const auto g = getTriGaussPoint(triPts, 3);
        for (std::size_t i = 0; i < g.points.size(); ++i) {
            gaussPoints_.push_back(g.points[i]);
            gaussWeight_.push_back(g.weights[i]);
            gaussLevelSet_.push_back(1.0);
        }
    }
}

Eigen::MatrixXd HESolverXFEM2DBlendedElement::bMatrixForOneGaussPoint(const Vec2& gpts) const {
    const double ls = gaussLevelSet_.empty() ? 0.0 : gaussLevelSet_.front();
    return bMatrixForOneGaussPoint(gpts, ls);
}

Eigen::MatrixXd HESolverXFEM2DBlendedElement::bMatrixForOneGaussPoint(const Vec2& gpts, double gptsLevelSet) const {
    const auto stdB = HESolverXFEM2DElement::bMatrixForOneGaussPoint(gpts);
    Eigen::MatrixXd full = stdB;
    const auto stdN = HESolverXFEM2DElement::shapeMatrixForOneGaussPoint(gpts);

    for (int i = 0; i < 4; ++i) {
        if (nodes_[i]->classify() == HESolverXFEM2DNodeClass::Heaviside) {
            Eigen::Matrix<double, 3, 2> sub;
            const double first = stdB(0, i * 2);
            const double second = stdB(1, i * 2 + 1);
            const double bLevel = gptsLevelSet - nodes_[i]->levelSet();
            sub << first, 0.0,
                   0.0, second,
                   second, first;
            sub *= bLevel;
            appendColumns(full, sub);
        } else if (nodes_[i]->classify() == HESolverXFEM2DNodeClass::Asymptotic) {
            const double alpha = nodes_[i]->tipAngle();
            const Vec2 crack = nodes_[i]->nearestTip();
            const auto QT = rotation(alpha);
            const Vec2 xy = quadNaturalToGlobal(gpts, extractCoordinates(nodes_));
            const Vec2 xp = QT * (xy - crack);
            const auto deriv = branchDerivative(xp.norm(), std::atan2(xp.y(), xp.x()), alpha);
            const auto& Br = deriv[0];
            const auto& dBdx = deriv[1];
            const auto& dBdy = deriv[2];
            const Vec2 xpi = QT * (nodes_[i]->coordinate() - crack);
            const auto BrI = branch(xpi.norm(), std::atan2(xpi.y(), xpi.x()));
            for (int mode = 0; mode < 4; ++mode) {
                Eigen::Matrix<double, 3, 2> sub;
                const double first = stdB(0, i * 2) * (Br(mode) - BrI(mode)) + stdN(0, i * 2) * dBdx(mode);
                const double second = stdB(1, i * 2 + 1) * (Br(mode) - BrI(mode)) + stdN(1, i * 2 + 1) * dBdy(mode);
                sub << first, 0.0,
                       0.0, second,
                       second, first;
                appendColumns(full, sub);
            }
        }
    }
    return full;
}

Eigen::MatrixXd HESolverXFEM2DBlendedElement::shapeMatrixForOneGaussPoint(const Vec2& gpts, double gptsLevelSet) const {
    const auto stdN = HESolverXFEM2DElement::shapeMatrixForOneGaussPoint(gpts);
    Eigen::MatrixXd full = stdN;
    for (int i = 0; i < 4; ++i) {
        if (nodes_[i]->classify() == HESolverXFEM2DNodeClass::Heaviside) {
            Eigen::Matrix<double, 2, 2> sub = Eigen::Matrix<double, 2, 2>::Zero();
            const double shape = stdN(0, i * 2);
            const double level = gptsLevelSet - nodes_[i]->levelSet();
            sub(0, 0) = shape * level;
            sub(1, 1) = shape * level;
            appendColumns(full, sub);
        } else if (nodes_[i]->classify() == HESolverXFEM2DNodeClass::Asymptotic) {
            const double alpha = nodes_[i]->tipAngle();
            const Vec2 crack = nodes_[i]->nearestTip();
            const auto QT = rotation(alpha);
            const Vec2 xy = quadNaturalToGlobal(gpts, extractCoordinates(nodes_));
            const Vec2 xp = QT * (xy - crack);
            const auto Br = branch(xp.norm(), std::atan2(xp.y(), xp.x()));
            const Vec2 xpi = QT * (nodes_[i]->coordinate() - crack);
            const auto BrI = branch(xpi.norm(), std::atan2(xpi.y(), xpi.x()));
            for (int mode = 0; mode < 4; ++mode) {
                Eigen::Matrix<double, 2, 2> sub = Eigen::Matrix<double, 2, 2>::Zero();
                const double value = stdN(0, i * 2) * (Br(mode) - BrI(mode));
                sub(0, 0) = value;
                sub(1, 1) = value;
                appendColumns(full, sub);
            }
        }
    }
    return full;
}

void HESolverXFEM2DBlendedElement::createStiffnessMatrix() {
    stiffnessMatrix_ = Eigen::MatrixXd::Zero(dofs_, dofs_);
    bMatrixForAllGaussPoint_.clear();
    for (std::size_t i = 0; i < gaussPoints_.size(); ++i) {
        const auto B = bMatrixForOneGaussPoint(gaussPoints_[i], gaussLevelSet_[i]);
        bMatrixForAllGaussPoint_.push_back(B);
        const auto J = jacobianForOneGaussPoint(gaussPoints_[i]);
        stiffnessMatrix_ += thickness_ * J.determinant() * gaussWeight_[i] * (B.transpose() * dMatrix_ * B);
    }
}

void HESolverXFEM2DBlendedElement::createMassMatrix() {
    massMatrix_ = Eigen::MatrixXd::Zero(dofs_, dofs_);
    for (std::size_t i = 0; i < gaussPoints_.size(); ++i) {
        const auto N = shapeMatrixForOneGaussPoint(gaussPoints_[i], gaussLevelSet_[i]);
        const auto J = jacobianForOneGaussPoint(gaussPoints_[i]);
        massMatrix_ += thickness_ * density_ * J.determinant() * gaussWeight_[i] * (N.transpose() * N);
    }
}

void HESolverXFEM2DBlendedElement::createDisplacement(const Eigen::VectorXd& displacementOfElement) {
    displacement_ = displacementOfElement;
    const bool hasStandard = std::any_of(nodes_.begin(), nodes_.end(), [](const HESolverXFEM2DNodePtr& node) { return node->classify() == HESolverXFEM2DNodeClass::Standard; });
    if (hasStandard) {
        HESolverXFEM2DElement::createDisplacement(displacementOfElement);
        return;
    }

    firstSubPolygonDisplacement_.clear();
    secondSubPolygonDisplacement_.clear();
    for (const auto& p : firstSubPolygon_) {
        const auto v = shapeMatrixForOneGaussPoint(p, -1.0) * displacementOfElement;
        firstSubPolygonDisplacement_.push_back(v);
    }
    for (const auto& p : secondSubPolygon_) {
        const auto v = shapeMatrixForOneGaussPoint(p, 1.0) * displacementOfElement;
        secondSubPolygonDisplacement_.push_back(v);
    }
}

void HESolverXFEM2DBlendedElement::createStress() {
    createGlobalGaussPoint();
    stress_.clear();
    for (std::size_t i = 0; i < gaussPoints_.size(); ++i) {
        const Vec3 sigma = dMatrix_ * (bMatrixForAllGaussPoint_[i] * displacement_);
        stress_.push_back(stressWithVonMises(sigma));
    }
    const std::vector<Vec2> nodeGaussPoint = {Vec2(-1, -1), Vec2(1, -1), Vec2(1, 1), Vec2(-1, 1)};
    double alternativeLevelSet = 1.0;
    for (int i = 0; i < 4; ++i) {
        if (!almostEqual(nodes_[i]->levelSet(), 0.0)) {
            alternativeLevelSet = nodes_[i]->levelSet();
            break;
        }
    }
    for (int i = 0; i < 4; ++i) {
        const double ls = almostEqual(nodes_[i]->levelSet(), 0.0) ? alternativeLevelSet : nodes_[i]->levelSet();
        accumulateNodeStress(dMatrix_, bMatrixForOneGaussPoint(nodeGaussPoint[i], ls), displacement_, nodes_[i]);
    }
}

} // namespace xfem
