#pragma once
#include "HESolveLinearAnalysisCommon.h"
#include "HESolveLinearAnalysisNode.h"
#include "HESolveLinearAnalysisTools.h"

namespace FEMLinearStatic {

enum class SolveElementType {
    Tet4,
    Prism6,
    Hex8
};

class HESolveLinearAnalysisElement {
public:
    HESolveLinearAnalysisElement(std::vector<HESolveLinearAnalysisNodePtr> nodes, const Mat66& dMatrix, double density = 0.0);
    virtual ~HESolveLinearAnalysisElement() = default;

    virtual void createGaussPoint() = 0;
    virtual void createStiffnessMatrix() = 0;
    virtual void createMassMatrix() = 0;
    virtual void createDisplacement(const Eigen::VectorXd& displacementOfElement);
    virtual void createStress();

    virtual int localDofs() const = 0;
    virtual SolveElementType kind() const = 0;
    virtual Eigen::MatrixXd bMatrixForNaturalPoint(const Vec3& natural) const = 0;
    virtual Eigen::MatrixXd shapeMatrixForNaturalPoint(const Vec3& natural) const = 0;

    const std::vector<HESolveLinearAnalysisNodePtr>& nodes() const noexcept { return nodes_; }
    const Mat66& dMatrix() const noexcept { return dMatrix_; }
    double density() const noexcept { return density_; }

    const Eigen::MatrixXd& stiffnessMatrix() const noexcept { return stiffnessMatrix_; }
    const Eigen::MatrixXd& massMatrix() const noexcept { return massMatrix_; }
    const std::vector<Vec3>& gaussPoints() const noexcept { return gaussPoints_; }
    const std::vector<double>& gaussWeights() const noexcept { return gaussWeights_; }
    const std::vector<Eigen::MatrixXd>& bMatrixAllGaussPoints() const noexcept { return bMatrixAllGaussPoints_; }
    const std::vector<Vec7>& stress() const noexcept { return stress_; }
    const Eigen::VectorXd& displacement() const noexcept { return displacement_; }

    static Vec7 stressWithVonMises(const Vec6& sigma);

protected:
    void appendGaussStressToNodes();

    std::vector<HESolveLinearAnalysisNodePtr> nodes_;
    Mat66 dMatrix_;
    double density_;
    Eigen::MatrixXd stiffnessMatrix_;
    Eigen::MatrixXd massMatrix_;
    std::vector<Vec3> gaussPoints_;
    std::vector<double> gaussWeights_;
    std::vector<Eigen::MatrixXd> bMatrixAllGaussPoints_;
    std::vector<Vec7> stress_;
    Eigen::VectorXd displacement_;
};

class Tet4Element final : public HESolveLinearAnalysisElement {
public:
    Tet4Element(std::vector<HESolveLinearAnalysisNodePtr> nodes, const Mat66& dMatrix, double density = 0.0);

    int localDofs() const override { return 12; }
    SolveElementType kind() const override { return SolveElementType::Tet4; }

    void createGaussPoint() override;
    void createStiffnessMatrix() override;
    void createMassMatrix() override;
    void createStress() override;

    Eigen::MatrixXd bMatrixForNaturalPoint(const Vec3& natural) const override;
    Eigen::MatrixXd shapeMatrixForNaturalPoint(const Vec3& natural) const override;

private:
    Eigen::Matrix<double, 4, 4> coefficientMatrix() const;
};

class Prism6Element final : public HESolveLinearAnalysisElement {
public:
    Prism6Element(std::vector<HESolveLinearAnalysisNodePtr> nodes, const Mat66& dMatrix, double density = 0.0);

    int localDofs() const override { return 18; }
    SolveElementType kind() const override { return SolveElementType::Prism6; }

    void createGaussPoint() override;
    void createStiffnessMatrix() override;
    void createMassMatrix() override;

    Eigen::MatrixXd bMatrixForNaturalPoint(const Vec3& natural) const override;
    Eigen::MatrixXd shapeMatrixForNaturalPoint(const Vec3& natural) const override;

private:
    Eigen::Matrix<double, 3, 6> naturalDerivatives(const Vec3& natural) const;
    Eigen::Matrix3d jacobian(const Vec3& natural) const;
    std::array<double, 6> shapeValues(const Vec3& natural) const;
};

class Hex8Element final : public HESolveLinearAnalysisElement {
public:
    Hex8Element(std::vector<HESolveLinearAnalysisNodePtr> nodes, const Mat66& dMatrix, double density = 0.0);

    int localDofs() const override { return 24; }
    SolveElementType kind() const override { return SolveElementType::Hex8; }

    void createGaussPoint() override;
    void createStiffnessMatrix() override;
    void createMassMatrix() override;

    Eigen::MatrixXd bMatrixForNaturalPoint(const Vec3& natural) const override;
    Eigen::MatrixXd shapeMatrixForNaturalPoint(const Vec3& natural) const override;

private:
    static const std::array<Vec3, 8>& naturalCorners();
    Eigen::Matrix3d jacobian(const Vec3& natural) const;
};

using ElementPtr = std::shared_ptr<HESolveLinearAnalysisElement>;

} // namespace FEMLinearStatic
