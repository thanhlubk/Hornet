#include "HornetExecute/HESolveLinearAnalysisElement.h"

namespace FEMLinearStatic {
namespace {

void accumulateNodeStress(const Mat66& dMatrix,
                          const Eigen::MatrixXd& bMatrix,
                          const Eigen::VectorXd& displacement,
                          const HESolveLinearAnalysisNodePtr& node)
{
    const Vec6 sigma = dMatrix * (bMatrix * displacement);
    node->stressList().push_back(HESolveLinearAnalysisElement::stressWithVonMises(sigma));
}

} // namespace

HESolveLinearAnalysisElement::HESolveLinearAnalysisElement(std::vector<HESolveLinearAnalysisNodePtr> nodes, const Mat66& dMatrix, double density)
    : nodes_(std::move(nodes)),
      dMatrix_(dMatrix),
      density_(density) {}

void HESolveLinearAnalysisElement::createDisplacement(const Eigen::VectorXd& displacementOfElement) {
    displacement_ = displacementOfElement;
}

Vec7 HESolveLinearAnalysisElement::stressWithVonMises(const Vec6& sigma) {
    const double sx = sigma(0);
    const double sy = sigma(1);
    const double sz = sigma(2);
    const double txy = sigma(3);
    const double tyz = sigma(4);
    const double tzx = sigma(5);

    const double vm = std::sqrt(
        0.5 * ((sx - sy) * (sx - sy) + (sy - sz) * (sy - sz) + (sz - sx) * (sz - sx)) +
        3.0 * (txy * txy + tyz * tyz + tzx * tzx)
    );

    Vec7 out;
    out << sx, sy, sz, txy, tyz, tzx, vm;
    return out;
}

void HESolveLinearAnalysisElement::appendGaussStressToNodes() {
    if (kind() == SolveElementType::Tet4) {
        if (gaussPoints_.empty()) {
            return;
        }
        const Vec6 sigma = dMatrix_ * (bMatrixAllGaussPoints_.front() * displacement_);
        const Vec7 packed = stressWithVonMises(sigma);
        for (auto& node : nodes_) {
            node->stressList().push_back(packed);
        }
        return;
    }
    else if (kind() == SolveElementType::Prism6) {
        static const std::array<Vec3, 6> prismNodeNatural = {
            Vec3(0.0, 0.0, -1.0),
            Vec3(1.0, 0.0, -1.0),
            Vec3(0.0, 1.0, -1.0),
            Vec3(0.0, 0.0,  1.0),
            Vec3(1.0, 0.0,  1.0),
            Vec3(0.0, 1.0,  1.0)
        };
        for (int i = 0; i < 6; ++i) {
            accumulateNodeStress(dMatrix_, bMatrixForNaturalPoint(prismNodeNatural[i]), displacement_, nodes_[i]);
        }
        return;
    }
    else if (kind() == SolveElementType::Hex8) {
        static const std::array<Vec3, 8> hexNodeNatural = {
            Vec3(-1.0, -1.0, -1.0),
            Vec3( 1.0, -1.0, -1.0),
            Vec3( 1.0,  1.0, -1.0),
            Vec3(-1.0,  1.0, -1.0),
            Vec3(-1.0, -1.0,  1.0),
            Vec3( 1.0, -1.0,  1.0),
            Vec3( 1.0,  1.0,  1.0),
            Vec3(-1.0,  1.0,  1.0)
        };

        for (int i = 0; i < 8; ++i) {
            accumulateNodeStress(dMatrix_, bMatrixForNaturalPoint(hexNodeNatural[i]), displacement_, nodes_[i]);
        }
        return;
    }
}

void HESolveLinearAnalysisElement::createStress() {
    stress_.clear();
    for (std::size_t i = 0; i < gaussPoints_.size(); ++i) {
        const Vec6 sigma = dMatrix_ * (bMatrixAllGaussPoints_[i] * displacement_);
        stress_.push_back(stressWithVonMises(sigma));
    }
    appendGaussStressToNodes();
}

Tet4Element::Tet4Element(std::vector<HESolveLinearAnalysisNodePtr> nodes, const Mat66& dMatrix, double density)
    : HESolveLinearAnalysisElement(std::move(nodes), dMatrix, density) {}

Eigen::Matrix<double, 4, 4> Tet4Element::coefficientMatrix() const {
    Eigen::Matrix<double, 4, 4> A;
    for (int i = 0; i < 4; ++i) {
        A(i, 0) = 1.0;
        A(i, 1) = nodes_[i]->coordinate().x();
        A(i, 2) = nodes_[i]->coordinate().y();
        A(i, 3) = nodes_[i]->coordinate().z();
    }
    return A.inverse();
}

void Tet4Element::createGaussPoint() {
    gaussPoints_.clear();
    gaussWeights_.clear();
    gaussPoints_.push_back(Vec3(0.25, 0.25, 0.25));
    gaussWeights_.push_back(1.0);
}

Eigen::MatrixXd Tet4Element::bMatrixForNaturalPoint(const Vec3&) const {
    const auto invA = coefficientMatrix();

    Eigen::MatrixXd B = Eigen::MatrixXd::Zero(6, 12);
    for (int i = 0; i < 4; ++i) {
        const double dNdx = invA(1, i);
        const double dNdy = invA(2, i);
        const double dNdz = invA(3, i);
        const int c = i * 3;

        B(0, c)     = dNdx;
        B(1, c + 1) = dNdy;
        B(2, c + 2) = dNdz;

        B(3, c)     = dNdy;
        B(3, c + 1) = dNdx;

        B(4, c + 1) = dNdz;
        B(4, c + 2) = dNdy;

        B(5, c)     = dNdz;
        B(5, c + 2) = dNdx;
    }
    return B;
}

Eigen::MatrixXd Tet4Element::shapeMatrixForNaturalPoint(const Vec3& natural) const {
    const double r = natural.x();
    const double s = natural.y();
    const double t = natural.z();

    const std::array<double, 4> N = {
        1.0 - r - s - t,
        r,
        s,
        t
    };

    Eigen::MatrixXd M = Eigen::MatrixXd::Zero(3, 12);
    for (int i = 0; i < 4; ++i) {
        const int c = i * 3;
        M(0, c)     = N[i];
        M(1, c + 1) = N[i];
        M(2, c + 2) = N[i];
    }
    return M;
}

void Tet4Element::createStiffnessMatrix() {
    const double volume = tetVolume({nodes_[0]->coordinate(), nodes_[1]->coordinate(), nodes_[2]->coordinate(), nodes_[3]->coordinate()});
    const auto B = bMatrixForNaturalPoint(Vec3(0.25, 0.25, 0.25));
    stiffnessMatrix_ = volume * (B.transpose() * dMatrix_ * B);
    bMatrixAllGaussPoints_.clear();
    bMatrixAllGaussPoints_.push_back(B);
}

void Tet4Element::createMassMatrix() {
    const double volume = tetVolume({nodes_[0]->coordinate(), nodes_[1]->coordinate(), nodes_[2]->coordinate(), nodes_[3]->coordinate()});
    massMatrix_ = Eigen::MatrixXd::Zero(12, 12);
    const double factor = density_ * volume / 20.0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            const double coeff = (i == j) ? 2.0 : 1.0;
            massMatrix_.block<3, 3>(i * 3, j * 3) = coeff * Eigen::Matrix3d::Identity();
        }
    }
    massMatrix_ *= factor;
}

void Tet4Element::createStress() {
    stress_.clear();
    const auto B = bMatrixForNaturalPoint(Vec3(0.25, 0.25, 0.25));
    const Vec6 sigma = dMatrix_ * (B * displacement_);
    const Vec7 packed = stressWithVonMises(sigma);
    stress_.push_back(packed);
    for (auto& node : nodes_) {
        node->stressList().push_back(packed);
    }
}

Prism6Element::Prism6Element(std::vector<HESolveLinearAnalysisNodePtr> nodes, const Mat66& dMatrix, double density)
    : HESolveLinearAnalysisElement(std::move(nodes), dMatrix, density) {}

std::array<double, 6> Prism6Element::shapeValues(const Vec3& natural) const {
    const double r = natural.x();
    const double s = natural.y();
    const double t = natural.z();

    return {
        0.5 * (1.0 - t) * (1.0 - r - s),
        0.5 * (1.0 - t) * r,
        0.5 * (1.0 - t) * s,
        0.5 * (1.0 + t) * (1.0 - r - s),
        0.5 * (1.0 + t) * r,
        0.5 * (1.0 + t) * s
    };
}

Eigen::Matrix<double, 3, 6> Prism6Element::naturalDerivatives(const Vec3& natural) const {
    const double r = natural.x();
    const double s = natural.y();
    const double t = natural.z();

    Eigen::Matrix<double, 3, 6> dN;
    dN <<
        -0.5 * (1.0 - t),  0.5 * (1.0 - t),                0.0, -0.5 * (1.0 + t),  0.5 * (1.0 + t),                0.0,
        -0.5 * (1.0 - t),                0.0,  0.5 * (1.0 - t), -0.5 * (1.0 + t),                0.0,  0.5 * (1.0 + t),
        -0.5 * (1.0 - r - s), -0.5 * r, -0.5 * s,  0.5 * (1.0 - r - s),  0.5 * r,  0.5 * s;
    return dN;
}

Eigen::Matrix3d Prism6Element::jacobian(const Vec3& natural) const {
    const auto dN = naturalDerivatives(natural);
    Eigen::Matrix<double, 6, 3> xyz;
    for (int i = 0; i < 6; ++i) {
        xyz(i, 0) = nodes_[i]->coordinate().x();
        xyz(i, 1) = nodes_[i]->coordinate().y();
        xyz(i, 2) = nodes_[i]->coordinate().z();
    }
    return dN * xyz;
}

void Prism6Element::createGaussPoint() {
    const auto data = getPrismGaussData3x2();
    gaussPoints_ = data.points;
    gaussWeights_ = data.weights;
}

Eigen::MatrixXd Prism6Element::bMatrixForNaturalPoint(const Vec3& natural) const {
    const auto dNnat = naturalDerivatives(natural);
    const auto J = jacobian(natural);
    const auto invJ = J.inverse();
    const Eigen::Matrix<double, 3, 6> dNxyz = invJ * dNnat;

    Eigen::MatrixXd B = Eigen::MatrixXd::Zero(6, 18);
    for (int i = 0; i < 6; ++i) {
        const double dNdx = dNxyz(0, i);
        const double dNdy = dNxyz(1, i);
        const double dNdz = dNxyz(2, i);
        const int c = i * 3;

        B(0, c)     = dNdx;
        B(1, c + 1) = dNdy;
        B(2, c + 2) = dNdz;

        B(3, c)     = dNdy;
        B(3, c + 1) = dNdx;

        B(4, c + 1) = dNdz;
        B(4, c + 2) = dNdy;

        B(5, c)     = dNdz;
        B(5, c + 2) = dNdx;
    }
    return B;
}

Eigen::MatrixXd Prism6Element::shapeMatrixForNaturalPoint(const Vec3& natural) const {
    const auto N = shapeValues(natural);
    Eigen::MatrixXd M = Eigen::MatrixXd::Zero(3, 18);
    for (int i = 0; i < 6; ++i) {
        const int c = i * 3;
        M(0, c)     = N[i];
        M(1, c + 1) = N[i];
        M(2, c + 2) = N[i];
    }
    return M;
}

void Prism6Element::createStiffnessMatrix() {
    stiffnessMatrix_ = Eigen::MatrixXd::Zero(18, 18);
    bMatrixAllGaussPoints_.clear();

    for (std::size_t i = 0; i < gaussPoints_.size(); ++i) {
        const auto B = bMatrixForNaturalPoint(gaussPoints_[i]);
        const auto J = jacobian(gaussPoints_[i]);
        const double detJ = J.determinant();
        bMatrixAllGaussPoints_.push_back(B);
        stiffnessMatrix_ += detJ * gaussWeights_[i] * (B.transpose() * dMatrix_ * B);
    }
}

void Prism6Element::createMassMatrix() {
    massMatrix_ = Eigen::MatrixXd::Zero(18, 18);

    for (std::size_t i = 0; i < gaussPoints_.size(); ++i) {
        const auto N = shapeMatrixForNaturalPoint(gaussPoints_[i]);
        const auto J = jacobian(gaussPoints_[i]);
        const double detJ = J.determinant();
        massMatrix_ += density_ * detJ * gaussWeights_[i] * (N.transpose() * N);
    }
}

Hex8Element::Hex8Element(std::vector<HESolveLinearAnalysisNodePtr> nodes, const Mat66& dMatrix, double density)
    : HESolveLinearAnalysisElement(std::move(nodes), dMatrix, density) {}

const std::array<Vec3, 8>& Hex8Element::naturalCorners() {
    static const std::array<Vec3, 8> corners = {
        Vec3(-1.0, -1.0, -1.0),
        Vec3( 1.0, -1.0, -1.0),
        Vec3( 1.0,  1.0, -1.0),
        Vec3(-1.0,  1.0, -1.0),
        Vec3(-1.0, -1.0,  1.0),
        Vec3( 1.0, -1.0,  1.0),
        Vec3( 1.0,  1.0,  1.0),
        Vec3(-1.0,  1.0,  1.0)
    };
    return corners;
}

Eigen::Matrix3d Hex8Element::jacobian(const Vec3& natural) const {
    const double xi = natural.x();
    const double eta = natural.y();
    const double zeta = natural.z();

    Eigen::Matrix<double, 3, 8> dN = Eigen::Matrix<double, 3, 8>::Zero();
    for (int i = 0; i < 8; ++i) {
        const double xi_i = naturalCorners()[i].x();
        const double eta_i = naturalCorners()[i].y();
        const double zeta_i = naturalCorners()[i].z();
        dN(0, i) = 0.125 * xi_i   * (1.0 + eta * eta_i) * (1.0 + zeta * zeta_i);
        dN(1, i) = 0.125 * eta_i  * (1.0 + xi  * xi_i)  * (1.0 + zeta * zeta_i);
        dN(2, i) = 0.125 * zeta_i * (1.0 + xi  * xi_i)  * (1.0 + eta  * eta_i);
    }

    Eigen::Matrix<double, 8, 3> xyz;
    for (int i = 0; i < 8; ++i) {
        xyz(i, 0) = nodes_[i]->coordinate().x();
        xyz(i, 1) = nodes_[i]->coordinate().y();
        xyz(i, 2) = nodes_[i]->coordinate().z();
    }
    return dN * xyz;
}

void Hex8Element::createGaussPoint() {
    const auto data = getHexGaussData2x2x2();
    gaussPoints_ = data.points;
    gaussWeights_ = data.weights;
}

Eigen::MatrixXd Hex8Element::bMatrixForNaturalPoint(const Vec3& natural) const {
    const double xi = natural.x();
    const double eta = natural.y();
    const double zeta = natural.z();

    Eigen::Matrix<double, 3, 8> dNnat = Eigen::Matrix<double, 3, 8>::Zero();
    for (int i = 0; i < 8; ++i) {
        const double xi_i = naturalCorners()[i].x();
        const double eta_i = naturalCorners()[i].y();
        const double zeta_i = naturalCorners()[i].z();
        dNnat(0, i) = 0.125 * xi_i   * (1.0 + eta * eta_i) * (1.0 + zeta * zeta_i);
        dNnat(1, i) = 0.125 * eta_i  * (1.0 + xi  * xi_i)  * (1.0 + zeta * zeta_i);
        dNnat(2, i) = 0.125 * zeta_i * (1.0 + xi  * xi_i)  * (1.0 + eta  * eta_i);
    }

    const Eigen::Matrix3d J = jacobian(natural);
    const Eigen::Matrix<double, 3, 8> dNxyz = J.inverse() * dNnat;

    Eigen::MatrixXd B = Eigen::MatrixXd::Zero(6, 24);
    for (int i = 0; i < 8; ++i) {
        const double dNdx = dNxyz(0, i);
        const double dNdy = dNxyz(1, i);
        const double dNdz = dNxyz(2, i);
        const int c = i * 3;

        B(0, c)     = dNdx;
        B(1, c + 1) = dNdy;
        B(2, c + 2) = dNdz;
        B(3, c)     = dNdy;
        B(3, c + 1) = dNdx;
        B(4, c + 1) = dNdz;
        B(4, c + 2) = dNdy;
        B(5, c)     = dNdz;
        B(5, c + 2) = dNdx;
    }
    return B;
}

Eigen::MatrixXd Hex8Element::shapeMatrixForNaturalPoint(const Vec3& natural) const {
    const double xi = natural.x();
    const double eta = natural.y();
    const double zeta = natural.z();

    Eigen::MatrixXd M = Eigen::MatrixXd::Zero(3, 24);
    for (int i = 0; i < 8; ++i) {
        const double xi_i = naturalCorners()[i].x();
        const double eta_i = naturalCorners()[i].y();
        const double zeta_i = naturalCorners()[i].z();
        const double N = 0.125 * (1.0 + xi * xi_i) * (1.0 + eta * eta_i) * (1.0 + zeta * zeta_i);
        const int c = i * 3;
        M(0, c)     = N;
        M(1, c + 1) = N;
        M(2, c + 2) = N;
    }
    return M;
}

void Hex8Element::createStiffnessMatrix() {
    stiffnessMatrix_ = Eigen::MatrixXd::Zero(24, 24);
    bMatrixAllGaussPoints_.clear();

    for (std::size_t i = 0; i < gaussPoints_.size(); ++i) {
        const auto B = bMatrixForNaturalPoint(gaussPoints_[i]);
        const auto J = jacobian(gaussPoints_[i]);
        const double detJ = J.determinant();
        bMatrixAllGaussPoints_.push_back(B);
        stiffnessMatrix_ += detJ * gaussWeights_[i] * (B.transpose() * dMatrix_ * B);
    }
}

void Hex8Element::createMassMatrix() {
    massMatrix_ = Eigen::MatrixXd::Zero(24, 24);

    for (std::size_t i = 0; i < gaussPoints_.size(); ++i) {
        const auto N = shapeMatrixForNaturalPoint(gaussPoints_[i]);
        const auto J = jacobian(gaussPoints_[i]);
        const double detJ = J.determinant();
        massMatrix_ += density_ * detJ * gaussWeights_[i] * (N.transpose() * N);
    }
}

} // namespace FEMLinearStatic
