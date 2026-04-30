#pragma once
#include "HESolveCrackPropagationCommon.h"
#include "HESolveCrackPropagationElement.h"
#include <eigen3/Eigen/Sparse>
#include <HornetBase/DatabaseSession.h>
#include <HornetBase/HIResult.h>

namespace XFEMCrackPropagation {

enum class HESolveCrackPropagationAnalysisType {
    Static,
    Modal
};

enum class HESolveCrackPropagationConditionType {
    PlaneStrain,
    PlaneStress
};

class HESolveCrackPropagationModel {
public:
    HESolveCrackPropagationModel(DatabaseSession* db,
          const std::vector<std::vector<Vec2>>& crack,
          const Eigen::Vector2d& properties,
          double thickness = 1.0,
          double density = 1000.0,
          HESolveCrackPropagationConditionType condition = HESolveCrackPropagationConditionType::PlaneStrain);

    void createStiffnessMatrix();
    void createMassMatrix();
    void solve(HESolveCrackPropagationAnalysisType analysis = HESolveCrackPropagationAnalysisType::Static, int numberOfMode = 5);
    void createDisplacement();
    void createStress();
    void createSIF(double radius, int tipIndex, double growthStepLength = 0.005);

    const std::vector<HESolveCrackPropagationNodePtr>& nodes() const noexcept { return nodes_; }
    const std::vector<HESolveCrackPropagationElementPtr>& elements() const noexcept { return elements_; }
    const std::vector<std::vector<int>>& booleanMap() const noexcept { return boolean_; }
    const Eigen::SparseMatrix<double>& stiffnessMatrix() const noexcept { return stiffnessMatrix_; }
    const Eigen::SparseMatrix<double>& massMatrix() const noexcept { return massMatrix_; }
    const Eigen::VectorXd& load() const noexcept { return load_; }
    const Eigen::VectorXd& boundaryCondition() const noexcept { return boundaryCondition_; }
    const Eigen::VectorXd& root() const noexcept { return root_; }
    const Eigen::VectorXd& displacement() const noexcept { return displacement_; }
    const Mat33& dMatrix() const noexcept { return dMatrix_; }

    bool getResultNode(HCursor* target, HIResultData& data) const noexcept;

    double K1() const noexcept { return K1_; }
    double K2() const noexcept { return K2_; }
    const std::vector<Vec2>& tip() const noexcept { return tip_; }
    const std::vector<double>& alpha() const noexcept { return alpha_; }
    const Vec2& nextPoint() const noexcept { return nextPoint_; }
    int dofs() const noexcept { return dofs_; }
    HESolveCrackPropagationConditionType condition() const noexcept { return condition_; }

    static bool isItInCircle(const std::vector<Vec2>& polygon, double radius, const Vec2& origin);
    std::vector<int> getElementOnCircle(double radius, const Vec2& origin) const;

private:
    Eigen::Vector2d properties_;
    HESolveCrackPropagationConditionType condition_;
    Mat33 dMatrix_ = Mat33::Zero();
    std::vector<std::vector<Vec2>> crack_;
    std::vector<HESolveCrackPropagationNodePtr> nodes_;
    std::vector<HESolveCrackPropagationElementPtr> elements_;
    std::vector<std::vector<int>> boolean_;
    int dofs_ = 0;
    std::vector<double> alpha_;
    std::vector<Vec2> tip_;

    Eigen::VectorXd load_;
    Eigen::VectorXd boundaryCondition_;
    Eigen::SparseMatrix<double> stiffnessMatrix_;
    Eigen::SparseMatrix<double> massMatrix_;
    Eigen::VectorXd root_;
    Eigen::VectorXd displacement_;

    double K1_ = 0.0;
    double K2_ = 0.0;
    Vec2 nextPoint_ = Vec2::Zero();

    // std::unordered_map<std::size_t, std::uint64_t> indexToNodeId_;
    std::unordered_map<std::uint64_t, std::size_t> nodeIdToIndex_;
};

} // namespace XFEMCrackPropagation
