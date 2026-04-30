#pragma once
#include "HESolveCrackPropagationCommon.h"
#include "HESolveCrackPropagationNode.h"
#include "HESolveCrackPropagationTools.h"

namespace XFEMCrackPropagation {

class HESolveCrackPropagationElement {
public:
    HESolveCrackPropagationElement(std::vector<HESolveCrackPropagationNodePtr> nodes, const Mat33& dMatrix, double thickness = 1.0, double density = 0.0);
    virtual ~HESolveCrackPropagationElement() = default;

    virtual void createGaussPoint() = 0;
    virtual void createStiffnessMatrix() = 0;
    virtual void createMassMatrix() = 0;
    virtual void createDisplacement(const Eigen::VectorXd& displacementOfElement);
    virtual void createStress();

    const std::vector<HESolveCrackPropagationNodePtr>& nodes() const noexcept { return nodes_; }
    const Mat33& dMatrix() const noexcept { return dMatrix_; }
    double thickness() const noexcept { return thickness_; }
    double density() const noexcept { return density_; }

    const Eigen::MatrixXd& stiffnessMatrix() const noexcept { return stiffnessMatrix_; }
    const Eigen::MatrixXd& massMatrix() const noexcept { return massMatrix_; }
    const std::vector<Vec2>& gaussPoints() const noexcept { return gaussPoints_; }
    const std::vector<double>& gaussWeight() const noexcept { return gaussWeight_; }
    const std::vector<Eigen::MatrixXd>& bMatrixForAllGaussPoint() const noexcept { return bMatrixForAllGaussPoint_; }
    const std::vector<Vec4>& stress() const noexcept { return stress_; }
    const std::vector<Vec2>& globalGaussPoints() const noexcept { return globalGaussPoints_; }
    const Eigen::VectorXd& displacement() const noexcept { return displacement_; }
    const std::string& classify() const noexcept { return classify_; }

    virtual int localDofs() const = 0;

    Mat22 jacobianForOneGaussPoint(const Vec2& gpts) const;
    static Mat28 shapeMatrixForOneGaussPoint(const Vec2& gpts);
    virtual Eigen::MatrixXd bMatrixForOneGaussPoint(const Vec2& gpts) const;

    static Vec4 stressWithVonMises(const Vec3& sigma);

protected:
    void createGlobalGaussPoint();
    static void appendColumns(Eigen::MatrixXd& dst, const Eigen::MatrixXd& src);

    std::vector<HESolveCrackPropagationNodePtr> nodes_;
    Mat33 dMatrix_;
    double thickness_;
    double density_;
    Eigen::MatrixXd stiffnessMatrix_;
    Eigen::MatrixXd massMatrix_;
    std::vector<Vec2> gaussPoints_;
    std::vector<double> gaussWeight_;
    std::vector<Eigen::MatrixXd> bMatrixForAllGaussPoint_;
    std::vector<Vec4> stress_;
    std::vector<Vec2> globalGaussPoints_;
    std::string classify_;
    Eigen::VectorXd displacement_;
};

class HESolveCrackPropagationStandardElement final : public HESolveCrackPropagationElement {
public:
    using HESolveCrackPropagationElement::HESolveCrackPropagationElement;
    int localDofs() const override { return 8; }
    void createGaussPoint() override;
    void createStiffnessMatrix() override;
    void createMassMatrix() override;
};

class HESolveCrackPropagationCrackBodyElement final : public HESolveCrackPropagationElement {
public:
    HESolveCrackPropagationCrackBodyElement(std::vector<HESolveCrackPropagationNodePtr> nodes, const Mat33& dMatrix, std::vector<Vec2> crack, double thickness = 1.0, double density = 0.0);

    int localDofs() const override { return 16; }
    void createGaussPoint() override;
    Eigen::MatrixXd bMatrixForOneGaussPoint(const Vec2& gpts) const override;
    Eigen::MatrixXd bMatrixForOneGaussPoint(const Vec2& gpts, double gptsLevelSet) const;
    Eigen::MatrixXd shapeMatrixForOneGaussPoint(const Vec2& gpts, double gptsLevelSet) const;
    void createStiffnessMatrix() override;
    void createMassMatrix() override;
    void createDisplacement(const Eigen::VectorXd& displacementOfElement) override;
    void createStress() override;

    const std::vector<Vec2>& crack() const noexcept { return crack_; }

private:
    std::vector<Vec2> crack_;
    std::vector<double> gaussLevelSet_;
    std::vector<Vec2> firstSubPolygon_;
    std::vector<Vec2> secondSubPolygon_;
    std::vector<Vec2> firstSubPolygonGlb_;
    std::vector<Vec2> secondSubPolygonGlb_;
    std::vector<Vec2> firstSubPolygonDisplacement_;
    std::vector<Vec2> secondSubPolygonDisplacement_;
};

class HESolveCrackPropagationCrackTipElement final : public HESolveCrackPropagationElement {
public:
    HESolveCrackPropagationCrackTipElement(std::vector<HESolveCrackPropagationNodePtr> nodes, const Mat33& dMatrix, std::vector<Vec2> crack, double thickness = 1.0, double density = 0.0);

    int localDofs() const override { return 40; }
    void createGaussPoint() override;
    Eigen::MatrixXd bMatrixForOneGaussPoint(const Vec2& gpts) const override;
    Eigen::MatrixXd shapeMatrixForOneGaussPoint(const Vec2& gpts) const;
    void createStiffnessMatrix() override;
    void createMassMatrix() override;

    static Eigen::Vector4d branch(double r, double theta);
    static std::array<Eigen::Vector4d, 3> branchDerivative(double r, double theta, double alpha);

    const std::vector<Vec2>& crack() const noexcept { return crack_; }
    double alpha() const noexcept { return alpha_; }

private:
    std::vector<Vec2> crack_;
    double alpha_;
};

class HESolveCrackPropagationBlendedElement final : public HESolveCrackPropagationElement {
public:
    HESolveCrackPropagationBlendedElement(std::vector<HESolveCrackPropagationNodePtr> nodes, const Mat33& dMatrix, std::vector<Vec2> crack = {}, double thickness = 1.0, double density = 0.0);

    int localDofs() const override { return dofs_; }
    void createGaussPoint() override;
    Eigen::MatrixXd bMatrixForOneGaussPoint(const Vec2& gpts) const override;
    Eigen::MatrixXd bMatrixForOneGaussPoint(const Vec2& gpts, double gptsLevelSet) const;
    Eigen::MatrixXd shapeMatrixForOneGaussPoint(const Vec2& gpts, double gptsLevelSet) const;
    void createStiffnessMatrix() override;
    void createMassMatrix() override;
    void createDisplacement(const Eigen::VectorXd& displacementOfElement) override;
    void createStress() override;

    static Eigen::Vector4d branch(double r, double theta);
    static std::array<Eigen::Vector4d, 3> branchDerivative(double r, double theta, double alpha);

    const std::vector<Vec2>& crack() const noexcept { return crack_; }

private:
    std::vector<Vec2> crack_;
    std::vector<double> gaussLevelSet_;
    int dofs_ = 8;
    std::vector<Vec2> firstSubPolygon_;
    std::vector<Vec2> secondSubPolygon_;
    std::vector<Vec2> firstSubPolygonGlb_;
    std::vector<Vec2> secondSubPolygonGlb_;
    std::vector<Vec2> firstSubPolygonDisplacement_;
    std::vector<Vec2> secondSubPolygonDisplacement_;
};

using HESolveCrackPropagationElementPtr = std::shared_ptr<HESolveCrackPropagationElement>;

} // namespace XFEMCrackPropagation
