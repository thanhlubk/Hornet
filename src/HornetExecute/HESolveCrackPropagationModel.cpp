#include "HornetExecute/HESolveCrackPropagationModel.h"
#include <HornetBase/HIElement.h>
#include <HornetBase/HINode.h>
#include <HornetBase/HILbcConstraint.h>
#include <HornetBase/HILbcForce.h>
#include <eigen3/Eigen/Sparse>
#include <eigen3/Eigen/SparseCholesky>
#include <eigen3/Eigen/SparseLU>

namespace XFEMCrackPropagation {
constexpr double kPi = 3.141592653589793238462643383279502884;
namespace {

Eigen::Matrix2d rotationLocal(double alpha) {
    Eigen::Matrix2d QT;
    QT << std::cos(alpha), std::sin(alpha),
         -std::sin(alpha), std::cos(alpha);
    return QT;
}

HESolveCrackPropagationConditionType parseCondition(const HESolveCrackPropagationConditionType condition) {
    return condition;
}

Mat33 makeDMatrix(const Eigen::Vector2d& properties, HESolveCrackPropagationConditionType condition) {
    const double E = properties.x();
    const double v = properties.y();
    Mat33 D = Mat33::Zero();
    if (condition == HESolveCrackPropagationConditionType::PlaneStrain) {
        D << 1 - v, v, 0,
             v, 1 - v, 0,
             0, 0, (1 - 2 * v) / 2.0;
        D *= E / ((1 + v) * (1 - 2 * v));
    } else {
        D << 1, v, 0,
             v, 1, 0,
             0, 0, (1 - v) / 2.0;
        D *= E / (1 - v * v);
    }
    return D;
}

} // namespace

HESolveCrackPropagationModel::HESolveCrackPropagationModel(DatabaseSession* db,
             const std::vector<std::vector<Vec2>>& crack,
             const Eigen::Vector2d& properties,
             double thickness,
             double density,
             HESolveCrackPropagationConditionType condition)
    : condition_(parseCondition(condition)),
    dMatrix_(makeDMatrix(properties, condition_)),
    crack_(crack),
    properties_(properties)
{
    // --- Load element and node data from database session
    struct NodeRec { Vec2 xy; int kind = 0; };
    std::vector<NodeRec> nodeData;
    auto pPoolNode = db->getPoolUnique(CategoryType::CatNode);
    if (pPoolNode) {
        nodeData.reserve(pPoolNode->count());
        for (auto any : pPoolNode->range()) {
            auto crNode = std::launder(reinterpret_cast<HCursor *>(any));
            auto pNode = crNode->item<HINode>();
            if (pNode) {
                nodeIdToIndex_[pNode->id()] = nodeData.size();
                nodeData.push_back({Vec2(pNode->position().x, pNode->position().y), 0});
            }
        }
    }

    std::vector<std::array<std::uint64_t, 4>> elementData;
    auto pPoolElement = db->getPoolMix(CategoryType::CatElement);
    if (pPoolElement) {
        elementData.reserve(pPoolElement->count());
        for (auto any : pPoolElement->range()) {
            auto crElement = std::launder(reinterpret_cast<HCursor *>(any));
            auto pElement = crElement->item<HIElement>();
            if (pElement && pElement->elementType() == ElementType::ElementTypeQuad4) { // Only Quad4 for now
                auto elemNodes = pElement->nodes();
                elementData.push_back({
                    nodeIdToIndex_[elemNodes[0]->id()],
                    nodeIdToIndex_[elemNodes[1]->id()],
                    nodeIdToIndex_[elemNodes[2]->id()],
                    nodeIdToIndex_[elemNodes[3]->id()]
                });
            }
        }
    }

    for (const auto& elem : elementData) {
        const std::vector<Vec2> polygon = {nodeData[elem[0]].xy, nodeData[elem[1]].xy, nodeData[elem[2]].xy, nodeData[elem[3]].xy};
        for (const auto& crackPath : crack_) {
            if (isItIntersect(crackPath, polygon)) {
                const auto intersections = getIntersectionPoints(crackPath, polygon);
                if (intersections.size() == 2) {
                    for (int k = 0; k < 4; ++k) {
                        if (nodeData[elem[k]].kind != 2) {
                            nodeData[elem[k]].kind = 1;
                        }
                    }
                } else {
                    for (int k = 0; k < 4; ++k) {
                        nodeData[elem[k]].kind = 2;
                    }
                }
            }
        }
    }

    std::vector<int> stdIds, hevIds, asymIds;
    for (std::size_t i = 0; i < nodeData.size(); ++i) {
        if (nodeData[i].kind == 0) {
            nodes_.push_back(std::make_shared<HESolveCrackPropagationStdNode>(nodeData[i].xy.x(), nodeData[i].xy.y()));
            stdIds.push_back(static_cast<int>(i));
            dofs_ += 2;
        } else if (nodeData[i].kind == 1) {
            nodes_.push_back(std::make_shared<HESolveCrackPropagationHevNode>(nodeData[i].xy.x(), nodeData[i].xy.y()));
            hevIds.push_back(static_cast<int>(i));
            dofs_ += 4;
        } else {
            nodes_.push_back(std::make_shared<HESolveCrackPropagationAsymptNode>(nodeData[i].xy.x(), nodeData[i].xy.y()));
            asymIds.push_back(static_cast<int>(i));
            dofs_ += 10;
        }
    }

    for (const auto& elem : elementData) {
        const std::vector<HESolveCrackPropagationNodePtr> elemNodes = {nodes_[elem[0]], nodes_[elem[1]], nodes_[elem[2]], nodes_[elem[3]]};
        const bool allStd = std::all_of(elemNodes.begin(), elemNodes.end(), [](const HESolveCrackPropagationNodePtr& n) { return n->classify() == HESolveCrackPropagationNodeClass::Standard; });
        const bool allHev = std::all_of(elemNodes.begin(), elemNodes.end(), [](const HESolveCrackPropagationNodePtr& n) { return n->classify() == HESolveCrackPropagationNodeClass::Heaviside; });
        const bool allAsym = std::all_of(elemNodes.begin(), elemNodes.end(), [](const HESolveCrackPropagationNodePtr& n) { return n->classify() == HESolveCrackPropagationNodeClass::Asymptotic; });

        std::vector<int> subBool;
        for (int i = 0; i < 8; ++i) {
            subBool.push_back(elem[i / 2] * 2 + i % 2);
        }

        if (allStd) {
            elements_.push_back(std::make_shared<HESolveCrackPropagationStandardElement>(elemNodes, dMatrix_, thickness, density));
            boolean_.push_back(subBool);
            continue;
        }

        if (allHev) {
            for (int i = 0; i < 4; ++i) {
                const auto it = std::find(hevIds.begin(), hevIds.end(), elem[i]);
                const int idx = static_cast<int>(std::distance(hevIds.begin(), it));
                subBool.push_back(static_cast<int>(nodes_.size()) * 2 + idx * 2);
                subBool.push_back(static_cast<int>(nodes_.size()) * 2 + idx * 2 + 1);
            }
            std::vector<Vec2> insideCrack;
            for (const auto& crackPath : crack_) {
                std::vector<double> levelSet;
                for (int i = 0; i < 4; ++i) {
                    levelSet.push_back(getLevelSet(crackPath, nodes_[elem[i]]->coordinate()));
                }
                const double sum = std::accumulate(levelSet.begin(), levelSet.end(), 0.0);
                if (std::abs(sum) != 4.0) {
                    for (int i = 0; i < 4; ++i) {
                        nodes_[elem[i]]->setLevelSet(levelSet[i]);
                    }
                    insideCrack = getCrackInPolygon(crackPath, {nodes_[elem[0]]->coordinate(), nodes_[elem[1]]->coordinate(), nodes_[elem[2]]->coordinate(), nodes_[elem[3]]->coordinate()});
                    break;
                }
            }
            if (insideCrack.empty()) {
                elements_.push_back(std::make_shared<HESolveCrackPropagationBlendedElement>(elemNodes, dMatrix_, insideCrack, thickness, density));
            } else {
                elements_.push_back(std::make_shared<HESolveCrackPropagationCrackBodyElement>(elemNodes, dMatrix_, insideCrack, thickness, density));
            }
            boolean_.push_back(subBool);
            continue;
        }

        if (allAsym) {
            for (int i = 0; i < 4; ++i) {
                const auto it = std::find(asymIds.begin(), asymIds.end(), elem[i]);
                const int idx = static_cast<int>(std::distance(asymIds.begin(), it));
                for (int d = 0; d < 8; ++d) {
                    subBool.push_back(static_cast<int>(nodes_.size()) * 2 + static_cast<int>(hevIds.size()) * 2 + idx * 8 + d);
                }
            }
            std::vector<Vec2> insideCrack;
            bool found = false;
            for (const auto& crackPath : crack_) {
                std::vector<double> levelSet;
                for (int i = 0; i < 4; ++i) {
                    levelSet.push_back(getLevelSet(crackPath, nodes_[elem[i]]->coordinate()));
                }
                const double sum = std::accumulate(levelSet.begin(), levelSet.end(), 0.0);
                if (std::abs(sum) != 4.0) {
                    found = true;
                    for (int i = 0; i < 4; ++i) {
                        nodes_[elem[i]]->setLevelSet(levelSet[i]);
                    }
                    insideCrack = getCrackInPolygon(crackPath, {nodes_[elem[0]]->coordinate(), nodes_[elem[1]]->coordinate(), nodes_[elem[2]]->coordinate(), nodes_[elem[3]]->coordinate()});
                    break;
                }
            }
            if (!found && !crack_.empty()) {
                for (int i = 0; i < 4; ++i) {
                    nodes_[elem[i]]->setLevelSet(getLevelSet(crack_.front(), nodes_[elem[i]]->coordinate()));
                }
                insideCrack = getCrackInPolygon(crack_.front(), {nodes_[elem[0]]->coordinate(), nodes_[elem[1]]->coordinate(), nodes_[elem[2]]->coordinate(), nodes_[elem[3]]->coordinate()});
            }
            auto tipElem = std::make_shared<HESolveCrackPropagationCrackTipElement>(elemNodes, dMatrix_, insideCrack, thickness, density);
            elements_.push_back(tipElem);
            boolean_.push_back(subBool);
            alpha_.push_back(tipElem->alpha());
            tip_.push_back(insideCrack.back());
            continue;
        }

        for (int i = 0; i < 4; ++i) {
            if (nodes_[elem[i]]->classify() == HESolveCrackPropagationNodeClass::Heaviside) {
                const auto it = std::find(hevIds.begin(), hevIds.end(), elem[i]);
                const int idx = static_cast<int>(std::distance(hevIds.begin(), it));
                subBool.push_back(static_cast<int>(nodes_.size()) * 2 + idx * 2);
                subBool.push_back(static_cast<int>(nodes_.size()) * 2 + idx * 2 + 1);
            }
        }
        for (int i = 0; i < 4; ++i) {
            if (nodes_[elem[i]]->classify() == HESolveCrackPropagationNodeClass::Asymptotic) {
                const auto it = std::find(asymIds.begin(), asymIds.end(), elem[i]);
                const int idx = static_cast<int>(std::distance(asymIds.begin(), it));
                for (int d = 0; d < 8; ++d) {
                    subBool.push_back(static_cast<int>(nodes_.size()) * 2 + static_cast<int>(hevIds.size()) * 2 + idx * 8 + d);
                }
            }
        }

        std::vector<Vec2> insideCrack;
        for (const auto& crackPath : crack_) {
            const auto sub = getCrackInPolygon(crackPath, {nodes_[elem[0]]->coordinate(), nodes_[elem[1]]->coordinate(), nodes_[elem[2]]->coordinate(), nodes_[elem[3]]->coordinate()});
            insideCrack.insert(insideCrack.end(), sub.begin(), sub.end());
            if (!insideCrack.empty()) {
                for (int i = 0; i < 4; ++i) {
                    nodes_[elem[i]]->setLevelSet(getLevelSet(crackPath, nodes_[elem[i]]->coordinate()));
                }
                break;
            }
        }
        elements_.push_back(std::make_shared<HESolveCrackPropagationBlendedElement>(elemNodes, dMatrix_, insideCrack, thickness, density));
        boolean_.push_back(subBool);
    }

    load_ = Eigen::VectorXd::Zero(dofs_);
    boundaryCondition_ = Eigen::VectorXd::Ones(dofs_);

    auto pPoolLbc = db->getPoolMix(CategoryType::CatLbc);
    if (pPoolLbc)
    {
        for (auto any : pPoolLbc->range())
        {
            auto crLbc = std::launder(reinterpret_cast<HCursor*>(any));
            // auto pLbc = crLbc->item<HILbc>();
            // if (!pLbc) continue;

            if (crLbc->type() == ItemType::ItemLbcForce) {
                auto pForce = crLbc->item<HILbcForce>();
                double fx = pForce->force().x;
                double fy = pForce->force().y;
                auto targets = pForce->targets();
                for (auto crNode : targets)
                {
                    auto pNode = crNode->item<HINode>();
                    if (!pNode)
                        continue;

                    auto idx = nodeIdToIndex_[pNode->id()];
                    load_(idx * 2) = fx;
                    load_(idx * 2 + 1) = fy;
                }
            }
            else if (crLbc->type() == ItemType::ItemLbcConstraint)
            {
                auto pConstraint = crLbc->item<HILbcConstraint>();
                bool cx = pConstraint->hasDof(LbcConstraintDof::LbcConstraintDofTx);
                bool cy = pConstraint->hasDof(LbcConstraintDof::LbcConstraintDofTy);
                auto targets = pConstraint->targets();
                for (auto crNode : targets) {
                    auto pNode = crNode->item<HINode>();
                    if (!pNode)
                        continue;

                    auto idx = nodeIdToIndex_[pNode->id()];
                    if (cx) 
                        boundaryCondition_(idx * 2) = 0.0;
                    if (cy) 
                        boundaryCondition_(idx * 2 + 1) = 0.0;
                }
            }
        }
    }

    // indexToNodeId_.clear();
    // for (const auto& [id, idx] : nodeIdToIndex)
    // {
    //     indexToNodeId_[idx] = id;
    // }
}

void HESolveCrackPropagationModel::createStiffnessMatrix() {
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

void HESolveCrackPropagationModel::createMassMatrix() {
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

void HESolveCrackPropagationModel::solve(HESolveCrackPropagationAnalysisType analysis, int numberOfMode) {
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
            const int gr = static_cast<int>(it.row());
            const int gc = static_cast<int>(it.col());
            const int rr = dofToReduced[static_cast<std::size_t>(gr)];
            const int rc = dofToReduced[static_cast<std::size_t>(gc)];
            if (rr >= 0 && rc >= 0) {
                kTriplets.emplace_back(rr, rc, it.value());
            }
        }
    }

    Eigen::SparseMatrix<double> Kred(nFree, nFree);
    Kred.setFromTriplets(kTriplets.begin(), kTriplets.end(), [](double a, double b) { return a + b; });
    Kred.makeCompressed();

    root_ = Eigen::VectorXd::Zero(dofs_);
    if (analysis == HESolveCrackPropagationAnalysisType::Static) {
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
    } else {
        std::vector<Eigen::Triplet<double>> mTriplets;
        mTriplets.reserve(massMatrix_.nonZeros());

        for (int outer = 0; outer < massMatrix_.outerSize(); ++outer) {
            for (Eigen::SparseMatrix<double>::InnerIterator it(massMatrix_, outer); it; ++it) {
                const int gr = static_cast<int>(it.row());
                const int gc = static_cast<int>(it.col());
                const int rr = dofToReduced[static_cast<std::size_t>(gr)];
                const int rc = dofToReduced[static_cast<std::size_t>(gc)];
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
        root_.resize(modes);
        for (int i = 0; i < modes; ++i) {
            root_(i) = std::sqrt(std::max(vals(i), 0.0)) / (2.0 * kPi);
        }
    }
}

void HESolveCrackPropagationModel::createDisplacement() {
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
        Vec2 p = nodes_[i]->coordinate();
        if (static_cast<Eigen::Index>(i * 2 + 1) < displacement_.size())
        {
            auto x = displacement_(static_cast<Eigen::Index>(i * 2));
            auto y = displacement_(static_cast<Eigen::Index>(i * 2 + 1));

            nodes_[i]->setDisplacement(Vec2(x, y));
        }
    }
}

void HESolveCrackPropagationModel::createStress() {
    for (auto& element : elements_) {
        element->createStress();
    }
    for (auto& node : nodes_) {
        node->createStress();
    }
}

bool HESolveCrackPropagationModel::getResultNode(HCursor* target, HIResultData& data) const noexcept {
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
    data.displacement.translational = node->displacement().norm();
    data.stress.xx = node->stress().x();
    data.stress.yy = node->stress().y();
    data.stress.xy = node->stress().z();
    data.stress.vonMises = node->stress().w();
    return true;
}

bool HESolveCrackPropagationModel::isItInCircle(const std::vector<Vec2>& polygon, double radius, const Vec2& origin) {
    for (const auto& p : polygon) {
        if (distance(p, origin) > radius) {
            return false;
        }
    }
    return true;
}

std::vector<int> HESolveCrackPropagationModel::getElementOnCircle(double radius, const Vec2& origin) const {
    std::vector<int> nodesInside;
    for (std::size_t i = 0; i < nodes_.size(); ++i) {
        if (distance(nodes_[i]->coordinate(), origin) < radius) {
            nodesInside.push_back(static_cast<int>(i));
        }
    }
    std::vector<int> elemInside;
    for (std::size_t i = 0; i < boolean_.size(); ++i) {
        for (int node : nodesInside) {
            if (std::find(boolean_[i].begin(), boolean_[i].end(), node * 2) != boolean_[i].end()) {
                elemInside.push_back(static_cast<int>(i));
                break;
            }
        }
    }
    std::vector<int> onCircle;
    for (int idx : elemInside) {
        std::vector<Vec2> polygon;
        for (int i = 0; i < 4; ++i) {
            polygon.push_back(elements_[idx]->nodes()[i]->coordinate());
        }
        if (!isItInCircle(polygon, radius, origin)) {
            onCircle.push_back(idx);
        }
    }
    return onCircle;
}

void HESolveCrackPropagationModel::createSIF(double radius, int tipIndex, double growthStepLength) {
    const Vec2 origin = tip_.at(tipIndex);
    const double alpha = alpha_.at(tipIndex);
    const auto jIntegralElement = getElementOnCircle(radius, origin);
    const auto QT = rotationLocal(alpha);
    Eigen::Vector2d I = Eigen::Vector2d::Zero();

    for (int elemIdx : jIntegralElement) {
        std::vector<double> q(4, 0.0);
        for (int i = 0; i < 4; ++i) {
            if (distance(elements_[elemIdx]->nodes()[i]->coordinate(), origin) < radius) {
                q[i] = 1.0;
            }
        }

        Eigen::VectorXd U(boolean_[elemIdx].size());
        for (std::size_t i = 0; i < boolean_[elemIdx].size(); ++i) {
            U(static_cast<Eigen::Index>(i)) = root_(boolean_[elemIdx][i]);
        }

        const auto& gaussPts = elements_[elemIdx]->gaussPoints();
        const auto& gaussWeights = elements_[elemIdx]->gaussWeight();
        const auto& allB = elements_[elemIdx]->bMatrixForAllGaussPoint();
        for (std::size_t gp = 0; gp < gaussPts.size(); ++gp) {
            const auto J = elements_[elemIdx]->jacobianForOneGaussPoint(gaussPts[gp]);
            const double detJ = J.determinant();
            const auto& Be = allB[gp];
            std::vector<Vec2> dNdx(4, Vec2::Zero());
            for (int i = 0; i < 4; ++i) {
                dNdx[i](0) = Be(2, i * 2 + 1);
                dNdx[i](1) = Be(2, i * 2);
            }

            Eigen::Matrix2d H = Eigen::Matrix2d::Zero();
            const int half = static_cast<int>(Be.cols() / 2);
            for (int i = 0; i < half; ++i) {
                H(0, 0) += Be(0, i * 2) * U(i * 2);
                H(0, 1) += Be(1, i * 2 + 1) * U(i * 2);
                H(1, 1) += Be(2, i * 2) * U(i * 2 + 1);
                H(1, 0) += Be(2, i * 2 + 1) * U(i * 2 + 1);
            }

            Vec2 gradq = Vec2::Zero();
            for (int i = 0; i < 4; ++i) {
                gradq += q[i] * dNdx[i];
            }
            const Eigen::Vector3d epsilon = Be * U;
            const Eigen::Vector3d sigma = elements_[elemIdx]->dMatrix() * epsilon;
            const Vec2 gradqLoc = QT * gradq;
            const Eigen::Matrix2d gradDispLoc = QT * H * QT.transpose();
            Eigen::Matrix2d strainLoc;
            strainLoc << epsilon(0), epsilon(2) / 2.0,
                         epsilon(2) / 2.0, epsilon(1);
            strainLoc = QT * strainLoc * QT.transpose();
            Eigen::Matrix2d stressLoc;
            stressLoc << sigma(0), sigma(2),
                         sigma(2), sigma(1);
            stressLoc = QT * stressLoc * QT.transpose();

            const Vec2 xy = quadNaturalToGlobal(gaussPts[gp], {elements_[elemIdx]->nodes()[0]->coordinate(), elements_[elemIdx]->nodes()[1]->coordinate(), elements_[elemIdx]->nodes()[2]->coordinate(), elements_[elemIdx]->nodes()[3]->coordinate()});
            const Vec2 xp = QT * (xy - origin);
            const double r = xp.norm();
            const double theta = std::atan2(xp.y(), xp.x());
            const double mu = properties_.x() / (2.0 * (1.0 + properties_.y()));
            const double kappa = 3.0 - 4.0 * properties_.y();
            const double SQR = safeSqrt(r);
            const double CT = std::cos(theta);
            const double ST = std::sin(theta);
            const double CT2 = std::cos(theta / 2.0);
            const double ST2 = std::sin(theta / 2.0);
            const double C3T2 = std::cos(3.0 * theta / 2.0);
            const double S3T2 = std::sin(3.0 * theta / 2.0);
            const double drdx = CT;
            const double drdy = ST;
            const double dtdx = -ST / std::max(r, 1e-12);
            const double dtdy = CT / std::max(r, 1e-12);
            const double FACStress = std::sqrt(1.0 / (2.0 * kPi));
            const double FACDisp = FACStress / (2.0 * mu);

            for (int mode = 0; mode < 2; ++mode) {
                Eigen::Matrix2d auxStress = Eigen::Matrix2d::Zero();
                Eigen::Matrix2d auxGradDisp = Eigen::Matrix2d::Zero();
                Eigen::Matrix2d auxEps = Eigen::Matrix2d::Zero();
                if (mode == 0) {
                    auxStress(0, 0) = FACStress / std::max(SQR, 1e-12) * CT2 * (1 - ST2 * S3T2);
                    auxStress(1, 1) = FACStress / std::max(SQR, 1e-12) * CT2 * (1 + ST2 * S3T2);
                    auxStress(0, 1) = FACStress / std::max(SQR, 1e-12) * ST2 * CT2 * C3T2;
                    auxStress(1, 0) = auxStress(0, 1);

                    const double u1 = FACDisp * SQR * CT2 * (kappa - CT);
                    const double du1dr = FACDisp * 0.5 / std::max(SQR, 1e-12) * CT2 * (kappa - CT);
                    const double du1dt = FACDisp * SQR * (-0.5 * ST2 * (kappa - CT) + CT2 * ST);
                    const double u2 = FACDisp * SQR * ST2 * (kappa - CT);
                    const double du2dr = FACDisp * 0.5 / std::max(SQR, 1e-12) * ST2 * (kappa - CT);
                    const double du2dt = FACDisp * SQR * (0.5 * CT2 * (kappa - CT) + ST2 * ST);
                    (void)u1; (void)u2;
                    auxGradDisp(0, 0) = du1dr * drdx + du1dt * dtdx;
                    auxGradDisp(0, 1) = du1dr * drdy + du1dt * dtdy;
                    auxGradDisp(1, 0) = du2dr * drdx + du2dt * dtdx;
                    auxGradDisp(1, 1) = du2dr * drdy + du2dt * dtdy;
                } else {
                    auxStress(0, 0) = -FACStress / std::max(SQR, 1e-12) * ST2 * (2 - CT2 * C3T2);
                    auxStress(1, 1) = FACStress / std::max(SQR, 1e-12) * ST2 * CT2 * C3T2;
                    auxStress(0, 1) = FACStress / std::max(SQR, 1e-12) * CT2 * (1 - ST2 * S3T2);
                    auxStress(1, 0) = auxStress(0, 1);

                    const double du1dr = FACDisp * 0.5 / std::max(SQR, 1e-12) * ST2 * (kappa + 2 + CT);
                    const double du1dt = FACDisp * SQR * (0.5 * CT2 * (kappa + 2 + CT) - ST2 * ST);
                    const double du2dr = -FACDisp * 0.5 / std::max(SQR, 1e-12) * CT2 * (kappa - 2 + CT);
                    const double du2dt = -FACDisp * SQR * (-0.5 * ST2 * (kappa - 2 + CT) - CT2 * ST);
                    auxGradDisp(0, 0) = du1dr * drdx + du1dt * dtdx;
                    auxGradDisp(0, 1) = du1dr * drdy + du1dt * dtdy;
                    auxGradDisp(1, 0) = du2dr * drdx + du2dt * dtdx;
                    auxGradDisp(1, 1) = du2dr * drdy + du2dt * dtdy;
                }
                auxEps(0, 0) = auxGradDisp(0, 0);
                auxEps(1, 0) = 0.5 * (auxGradDisp(1, 0) + auxGradDisp(0, 1));
                auxEps(0, 1) = auxEps(1, 0);
                auxEps(1, 1) = auxGradDisp(1, 1);

                const double I1 = (stressLoc(0, 0) * auxGradDisp(0, 0) + stressLoc(1, 0) * auxGradDisp(1, 0)) * gradqLoc(0) +
                                  (stressLoc(0, 1) * auxGradDisp(0, 0) + stressLoc(1, 1) * auxGradDisp(1, 0)) * gradqLoc(1);
                const double I2 = (auxStress(0, 0) * gradDispLoc(0, 0) + auxStress(1, 0) * gradDispLoc(1, 0)) * gradqLoc(0) +
                                  (auxStress(1, 0) * gradDispLoc(0, 0) + auxStress(1, 1) * gradDispLoc(1, 0)) * gradqLoc(1);
                double strainEnergy = 0.0;
                for (int i = 0; i < 2; ++i) {
                    for (int j = 0; j < 2; ++j) {
                        strainEnergy += auxStress(i, j) * strainLoc(i, j);
                    }
                }
                I(mode) += (I1 + I2 - strainEnergy * gradqLoc(0)) * detJ * gaussWeights[gp];
            }
        }
    }

    if (condition_ == HESolveCrackPropagationConditionType::PlaneStrain) {
        K1_ = I(0) * properties_.x() / (2.0 * (1.0 - properties_.y() * properties_.y()));
        K2_ = I(1) * properties_.x() / (2.0 * (1.0 - properties_.y() * properties_.y()));
    } else {
        K1_ = I(0) * properties_.x();
        K2_ = I(1) * properties_.x();
    }
    const double teta = 2.0 * std::atan((-2.0 * K2_ / K1_) / (1.0 + std::sqrt(1.0 + 8.0 * (K2_ / K1_) * (K2_ / K1_))));
    const double gama = alpha + teta;
    nextPoint_ = Vec2(growthStepLength * std::cos(gama) + origin.x(), growthStepLength * std::sin(gama) + origin.y());
}

} // namespace XFEMCrackPropagation
