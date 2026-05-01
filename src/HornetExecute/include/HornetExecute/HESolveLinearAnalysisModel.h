#pragma once
#include "HESolveLinearAnalysisCommon.h"
#include "HESolveLinearAnalysisElement.h"
#include <HornetBase/DatabaseSession.h>
#include <eigen3/Eigen/Sparse>
#include <HornetBase/HIResult.h>
#include "HESolveDef.h"

namespace FEMLinearStatic {

class HESolveLinearAnalysisModel {
public:
    HESolveLinearAnalysisModel(DatabaseSession* db,
            double youngsModulus,
            double poissonRatio,
            double density = 0.0);

    void createStiffnessMatrix();
    void createMassMatrix();
    void solve(HESolve::AnalysisType analysis = HESolve::AnalysisType::Static, int numberOfMode = 5);
    void createDisplacement(int mode = 0);
    void createStress();

    const std::vector<HESolveLinearAnalysisNodePtr>& nodes() const noexcept { return nodes_; }
    const std::vector<ElementPtr>& elements() const noexcept { return elements_; }
    const std::vector<std::vector<int>>& booleanMap() const noexcept { return boolean_; }

    const Eigen::SparseMatrix<double>& stiffnessMatrix() const noexcept { return stiffnessMatrix_; }
    const Eigen::SparseMatrix<double>& massMatrix() const noexcept { return massMatrix_; }
    const Eigen::VectorXd& load() const noexcept { return load_; }
    const Eigen::VectorXd& boundaryCondition() const noexcept { return boundaryCondition_; }
    const Eigen::VectorXd& root() const noexcept { return root_; }
    const Eigen::VectorXd& displacement() const noexcept { return displacement_; }
    const Mat66& dMatrix() const noexcept { return dMatrix_; }
    int dofs() const noexcept { return dofs_; }

    bool getResultNode(HCursor* target, HIResultData& data) const noexcept;

    const Eigen::VectorXd& frequency() const noexcept { return frequency_; }

private:
    Mat66 dMatrix_ = Mat66::Zero();
    std::vector<HESolveLinearAnalysisNodePtr> nodes_;
    std::vector<ElementPtr> elements_;
    std::vector<std::vector<int>> boolean_;
    int dofs_ = 0;

    Eigen::VectorXd load_;
    Eigen::VectorXd boundaryCondition_;
    Eigen::SparseMatrix<double> stiffnessMatrix_;
    Eigen::SparseMatrix<double> massMatrix_;
    Eigen::VectorXd root_;
    Eigen::VectorXd displacement_;

    std::unordered_map<std::uint64_t, std::size_t> nodeIdToIndex_;
    Eigen::VectorXd frequency_;
    std::vector<Eigen::VectorXd> modeShapes_;
    HESolve::AnalysisType analysisType_;
};

} // namespace FEMLinearStatic
