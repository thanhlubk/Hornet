#include "HornetExecute/HESolveLinearAnalysisModel.h"
#include <eigen3/Eigen/Sparse>
#include <eigen3/Eigen/SparseCholesky>
#include <eigen3/Eigen/SparseLU>
#include <HornetBase/HIElement.h>
#include <HornetBase/HINode.h>
#include <HornetBase/HILbcConstraint.h>
#include <HornetBase/HILbcForce.h>

namespace FEMLinearStatic {
namespace {

Mat66 makeDMatrix(double youngsModulus, double poissonRatio) {
    const double E = youngsModulus;
    const double v = poissonRatio;
    const double lambda = E * v / ((1.0 + v) * (1.0 - 2.0 * v));
    const double mu = E / (2.0 * (1.0 + v));

    Mat66 D = Mat66::Zero();
    D(0, 0) = lambda + 2.0 * mu;
    D(0, 1) = lambda;
    D(0, 2) = lambda;
    D(1, 0) = lambda;
    D(1, 1) = lambda + 2.0 * mu;
    D(1, 2) = lambda;
    D(2, 0) = lambda;
    D(2, 1) = lambda;
    D(2, 2) = lambda + 2.0 * mu;
    D(3, 3) = mu;
    D(4, 4) = mu;
    D(5, 5) = mu;
    return D;
}

bool isBlank(const std::string& s) {
    return trim(s).empty();
}

} // namespace

HESolveLinearAnalysisModel::HESolveLinearAnalysisModel(DatabaseSession* db,
                 double youngsModulus,
                 double poissonRatio,
                 double density)
    : dMatrix_(makeDMatrix(youngsModulus, poissonRatio))
{
    auto pPoolNode = db->getPoolUnique(CategoryType::CatNode);
    if (pPoolNode) {
        nodes_.reserve(pPoolNode->count());
        for (auto any : pPoolNode->range()) {
            auto crNode = std::launder(reinterpret_cast<HCursor *>(any));
            auto pNode = crNode->item<HINode>();
            if (pNode) {
                nodeIdToIndex_[pNode->id()] = nodes_.size();
                nodes_.push_back(std::make_shared<HESolveLinearAnalysisNode>(pNode->position().x, pNode->position().y, pNode->position().z));
            }
        }
    }
    dofs_ = static_cast<int>(nodes_.size()) * 3;

    auto pPoolElement = db->getPoolMix(CategoryType::CatElement);
    if (pPoolElement) {
        elements_.reserve(pPoolElement->count());
        boolean_.reserve(pPoolElement->count());

        for (auto any : pPoolElement->range()) {
            auto crElement = std::launder(reinterpret_cast<HCursor *>(any));
            auto pElement = crElement->item<HIElement>();
            if (pElement)
            {
                auto vecCrNode = pElement->nodes();
                std::vector<HESolveLinearAnalysisNodePtr> elemNodes;
                elemNodes.reserve(vecCrNode.size());
                for (auto crNode : vecCrNode) {
                    auto pNode = crNode->item<HINode>();
                    if (!pNode) {
                        throw std::runtime_error("Element node is not HINode");
                    }
                    elemNodes.push_back(nodes_[nodeIdToIndex_[pNode->id()]]);
                }

                if (pElement->elementType() == ElementType::ElementTypeHex8) {
                    elements_.push_back(std::make_shared<Hex8Element>(elemNodes, dMatrix_, density));
                }
                else if (pElement->elementType() == ElementType::ElementTypeTet4)
                {
                    elements_.push_back(std::make_shared<Tet4Element>(elemNodes, dMatrix_, density));
                }
                else if (pElement->elementType() == ElementType::ElementTypePrism6)
                {
                    elements_.push_back(std::make_shared<Prism6Element>(elemNodes, dMatrix_, density));
                }
                else
                {
                    throw std::runtime_error("Unsupported element type");
                }
                std::vector<int> map;
                map.reserve(vecCrNode.size() * 3);
                for (auto crNode : vecCrNode) {
                    auto pNode = crNode->item<HINode>();
                    if (!pNode) {
                        throw std::runtime_error("Element node is not HINode");
                    }
                    map.push_back(nodeIdToIndex_[pNode->id()] * 3);
                    map.push_back(nodeIdToIndex_[pNode->id()] * 3 + 1);
                    map.push_back(nodeIdToIndex_[pNode->id()] * 3 + 2);
                }
                boolean_.push_back(std::move(map));
            }
        }
    }

    load_ = Eigen::VectorXd::Zero(dofs_);
    boundaryCondition_ = Eigen::VectorXd::Ones(dofs_);
    auto pPoolLbc = db->getPoolMix(CategoryType::CatLbc);
    if (pPoolLbc)
    {
        for (auto any : pPoolLbc->range())
        {
            auto crLbc = std::launder(reinterpret_cast<HCursor*>(any));
            if (crLbc->type() == ItemType::ItemLbcForce) {
                auto pForce = crLbc->item<HILbcForce>();
                double fx = pForce->force().x;
                double fy = pForce->force().y;
                double fz = pForce->force().z;
                auto targets = pForce->targets();
                for (auto crNode : targets)
                {
                    auto pNode = crNode->item<HINode>();
                    if (!pNode)
                        continue;

                    auto idx = nodeIdToIndex_[pNode->id()];
                    load_(idx * 3) = fx;
                    load_(idx * 3 + 1) = fy;
                    load_(idx * 3 + 2) = fz;
                }
            }
            else if (crLbc->type() == ItemType::ItemLbcConstraint)
            {
                auto pConstraint = crLbc->item<HILbcConstraint>();
                bool cx = pConstraint->hasDof(LbcConstraintDof::LbcConstraintDofTx);
                bool cy = pConstraint->hasDof(LbcConstraintDof::LbcConstraintDofTy);
                bool cz = pConstraint->hasDof(LbcConstraintDof::LbcConstraintDofTz);
                auto targets = pConstraint->targets();
                for (auto crNode : targets) {
                    auto pNode = crNode->item<HINode>();
                    if (!pNode)
                        continue;

                    auto idx = nodeIdToIndex_[pNode->id()];
                    if (cx) 
                        boundaryCondition_(idx * 3) = 0.0;
                    if (cy) 
                        boundaryCondition_(idx * 3 + 1) = 0.0;
                    if (cz)
                        boundaryCondition_(idx * 3 + 2) = 0.0;
                }
            }
        }
    }
}

void HESolveLinearAnalysisModel::createStiffnessMatrix() {
    std::vector<Eigen::Triplet<double>> triplets;
    std::size_t reserveCount = 0;
    for (const auto& map : boolean_) {
        reserveCount += map.size() * map.size();
    }
    triplets.reserve(reserveCount);

    for (std::size_t i = 0; i < elements_.size(); ++i) {
        elements_[i]->createGaussPoint();
        elements_[i]->createStiffnessMatrix();

        const auto& Ke = elements_[i]->stiffnessMatrix();
        const auto& map = boolean_[i];
        for (Eigen::Index r = 0; r < Ke.rows(); ++r) {
            const int gr = map[static_cast<std::size_t>(r)];
            for (Eigen::Index c = 0; c < Ke.cols(); ++c) {
                const double value = Ke(r, c);
                if (std::abs(value) > 1e-14) {
                    const int gc = map[static_cast<std::size_t>(c)];
                    triplets.emplace_back(gr, gc, value);
                }
            }
        }
    }

    stiffnessMatrix_.resize(dofs_, dofs_);
    stiffnessMatrix_.setFromTriplets(triplets.begin(), triplets.end(), [](double a, double b) { return a + b; });
    stiffnessMatrix_.makeCompressed();
}

void HESolveLinearAnalysisModel::createMassMatrix() {
    std::vector<Eigen::Triplet<double>> triplets;
    std::size_t reserveCount = 0;
    for (const auto& map : boolean_) {
        reserveCount += map.size() * map.size();
    }
    triplets.reserve(reserveCount);

    for (std::size_t i = 0; i < elements_.size(); ++i) {
        elements_[i]->createMassMatrix();
        const auto& Me = elements_[i]->massMatrix();
        const auto& map = boolean_[i];
        for (Eigen::Index r = 0; r < Me.rows(); ++r) {
            const int gr = map[static_cast<std::size_t>(r)];
            for (Eigen::Index c = 0; c < Me.cols(); ++c) {
                const double value = Me(r, c);
                if (std::abs(value) > 1e-14) {
                    const int gc = map[static_cast<std::size_t>(c)];
                    triplets.emplace_back(gr, gc, value);
                }
            }
        }
    }

    massMatrix_.resize(dofs_, dofs_);
    massMatrix_.setFromTriplets(triplets.begin(), triplets.end(), [](double a, double b) { return a + b; });
    massMatrix_.makeCompressed();
}

void HESolveLinearAnalysisModel::solve(HESolve::AnalysisType analysis, int numberOfMode) {
    std::vector<int> freeDofs;
    freeDofs.reserve(dofs_);

    std::vector<int> dofToReduced(static_cast<std::size_t>(dofs_), -1);
    for (int i = 0; i < dofs_; ++i) {
        if (almostEqual(boundaryCondition_(i), 1.0)) {
            dofToReduced[static_cast<std::size_t>(i)] = static_cast<int>(freeDofs.size());
            freeDofs.push_back(i);
        }
    }

    const Eigen::Index nFree = static_cast<Eigen::Index>(freeDofs.size());
    Eigen::VectorXd Fred = Eigen::VectorXd::Zero(nFree);
    for (Eigen::Index i = 0; i < nFree; ++i) {
        Fred(i) = load_(freeDofs[static_cast<std::size_t>(i)]);
    }

    std::vector<Eigen::Triplet<double>> kTriplets;
    kTriplets.reserve(stiffnessMatrix_.nonZeros());
    for (int outer = 0; outer < stiffnessMatrix_.outerSize(); ++outer) {
        for (Eigen::SparseMatrix<double>::InnerIterator it(stiffnessMatrix_, outer); it; ++it) {
            const int rr = dofToReduced[static_cast<std::size_t>(it.row())];
            const int rc = dofToReduced[static_cast<std::size_t>(it.col())];
            if (rr >= 0 && rc >= 0) {
                kTriplets.emplace_back(rr, rc, it.value());
            }
        }
    }

    Eigen::SparseMatrix<double> Kred(nFree, nFree);
    Kred.setFromTriplets(kTriplets.begin(), kTriplets.end(), [](double a, double b) { return a + b; });
    Kred.makeCompressed();

    root_ = Eigen::VectorXd::Zero(dofs_);

    if (analysis == HESolve::AnalysisType::Static) {
        Eigen::VectorXd x;
        Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> ldlt;
        ldlt.compute(Kred);
        if (ldlt.info() == Eigen::Success) {
            x = ldlt.solve(Fred);
            if (ldlt.info() != Eigen::Success) {
                throw std::runtime_error("Sparse LDLT solve failed");
            }
        } else {
            Eigen::SparseLU<Eigen::SparseMatrix<double>> lu;
            lu.analyzePattern(Kred);
            lu.factorize(Kred);
            if (lu.info() != Eigen::Success) {
                throw std::runtime_error("Sparse LU factorization failed");
            }
            x = lu.solve(Fred);
            if (lu.info() != Eigen::Success) {
                throw std::runtime_error("Sparse LU solve failed");
            }
        }
        for (Eigen::Index i = 0; i < nFree; ++i) {
            root_(freeDofs[static_cast<std::size_t>(i)]) = x(i);
        }
        return;
    }

    std::vector<Eigen::Triplet<double>> mTriplets;
    mTriplets.reserve(massMatrix_.nonZeros());
    for (int outer = 0; outer < massMatrix_.outerSize(); ++outer) {
        for (Eigen::SparseMatrix<double>::InnerIterator it(massMatrix_, outer); it; ++it) {
            const int rr = dofToReduced[static_cast<std::size_t>(it.row())];
            const int rc = dofToReduced[static_cast<std::size_t>(it.col())];
            if (rr >= 0 && rc >= 0) {
                mTriplets.emplace_back(rr, rc, it.value());
            }
        }
    }

    Eigen::SparseMatrix<double> Mred(nFree, nFree);
    Mred.setFromTriplets(mTriplets.begin(), mTriplets.end(), [](double a, double b) { return a + b; });
    Mred.makeCompressed();

    Eigen::MatrixXd KredDense(Kred);
    Eigen::MatrixXd MredDense(Mred);
    Eigen::GeneralizedSelfAdjointEigenSolver<Eigen::MatrixXd> solver(KredDense, MredDense);
    if (solver.info() != Eigen::Success) {
        throw std::runtime_error("Generalized eigenvalue solve failed");
    }

    const auto& vals = solver.eigenvalues();
    const int modes = std::min<int>(numberOfMode, static_cast<int>(vals.size()));
    constexpr double kPi = 3.141592653589793238462643383279502884;
    root_.resize(modes);
    for (int i = 0; i < modes; ++i) {
        root_(i) = std::sqrt(std::max(vals(i), 0.0)) / (2.0 * kPi);
    }
}

void HESolveLinearAnalysisModel::createDisplacement() {
    displacement_ = root_;
    for (std::size_t i = 0; i < elements_.size(); ++i) {
        Eigen::VectorXd elemDisp(boolean_[i].size());
        for (std::size_t j = 0; j < boolean_[i].size(); ++j) {
            elemDisp(static_cast<Eigen::Index>(j)) = displacement_(boolean_[i][j]);
        }
        elements_[i]->createDisplacement(elemDisp);
    }

    for (std::size_t i = 0; i < nodes_.size(); ++i)
    {
        Vec3 p = nodes_[i]->coordinate();
        if (static_cast<Eigen::Index>(i * 3 + 2) < displacement_.size())
        {
            auto x = displacement_(static_cast<Eigen::Index>(i * 3));
            auto y = displacement_(static_cast<Eigen::Index>(i * 3 + 1));
            auto z = displacement_(static_cast<Eigen::Index>(i * 3 + 2));

            nodes_[i]->setDisplacement(Vec3(x, y, z));
        }
    }
}

void HESolveLinearAnalysisModel::createStress() {
    for (auto& element : elements_) {
        element->createStress();
    }
    for (auto& node : nodes_) {
        node->createStress();
    }
}

bool HESolveLinearAnalysisModel::getResultNode(HCursor* target, HIResultData& data) const noexcept {
    auto pNode = target->item<HINode>();
    if (!pNode)
        return false;

    const auto it = nodeIdToIndex_.find(pNode->id());
    if (it == nodeIdToIndex_.end())
        return false;

    const int idx = static_cast<int>(it->second);
    const auto& node = nodes_[static_cast<std::size_t>(idx)];
    data.displacement.x = node->displacement().x();
    data.displacement.y = node->displacement().y();
    data.displacement.z = node->displacement().z();
    data.stress.xx = node->stress()[0];
    data.stress.yy = node->stress()[1];
    data.stress.zz = node->stress()[2];
    data.stress.xy = node->stress()[3];
    data.stress.yz = node->stress()[4];
    data.stress.xz = node->stress()[5];
    data.stress.vonMises = node->stress()[6];
    return true;
}
} // namespace FEMLinearStatic
