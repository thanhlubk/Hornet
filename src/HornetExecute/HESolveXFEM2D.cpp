#include "HornetExecute/HESolveXFEM2D.h"
#include <HornetBase/HINode.h>
#include <HornetBase/HIElement.h>
#include <HornetBase/HILbcConstraint.h>
#include <HornetBase/HILbcForce.h>
#include "HornetExecute/HESolverXFEM2DModel.h"
#include "HornetExecute/HESolverXFEM2DCommon.h"

HESolveXFEM2D::HESolveXFEM2D(std::vector<std::vector<HVector2d>> vecCrack, double thickness, double density, double youngsModulus, double poissonRatio, XfemAnalysisType analysisType, XfemConditionType conditionType, double sifRadius, double growthStepLength, DatabaseSession* db, int step, bool transaction, bool logCommand)
    : HExecute(db, transaction, logCommand),
    m_vecCrack(std::move(vecCrack)),
    m_dThickness(thickness),
    m_dDensity(density),
    m_dYoungsModulus(youngsModulus),
    m_dPoissonRatio(poissonRatio),
    m_eAnalysisType(analysisType),
    m_eConditionType(conditionType),
    m_dSifRadius(sifRadius),
    m_dGrowthStepLength(growthStepLength),
    m_iStep(step)
{
}

HESolveXFEM2D::~HESolveXFEM2D()
{
}

void HESolveXFEM2D::logCommand()
{
    return;
}

bool HESolveXFEM2D::onExecute()
{
    std::vector<std::vector<xfem::Vec2>> vecDisplayedCracks;
    vecDisplayedCracks.reserve(m_vecCrack.size());
    for (const auto& line : m_vecCrack)
    {
        std::vector<xfem::Vec2> crack;
        crack.reserve(line.size());
        for (const auto& p : line) {
            crack.emplace_back(p.x, p.y);
        }
        vecDisplayedCracks.push_back(crack);
    }
    
    auto model = xfem::HESolverXFEM2DModel(m_pDb, vecDisplayedCracks, Eigen::Vector2d(m_dYoungsModulus, m_dPoissonRatio), m_dThickness, m_dDensity, static_cast<xfem::HESolverXFEM2DConditionType>(m_eConditionType));

    model.createStiffnessMatrix();
    model.solve(static_cast<xfem::HESolverXFEM2DAnalysisType>(m_eAnalysisType));
    model.createDisplacement();
    model.createStress();

    m_pDb->emplace<HIResult>(m_iStep + 1);
    auto pResult = m_pDb->checkOut<HIResult>(m_iStep + 1);
    if (pResult)
    {
        auto pPoolNode = m_pDb->getPoolUnique(CategoryType::CatNode);
        if (pPoolNode)
        {
            for (auto any : pPoolNode->range()) {
                auto crNode = std::launder(reinterpret_cast<HCursor *>(any));
                HIResultData data;
                if (model.getResultNode(crNode, data))
                {
                    pResult->setResult(crNode, data);
                }
            }
        }
    }

    for (auto i = 0; i < model.tip().size(); i++)
    {
        model.createSIF(m_dSifRadius, i, m_dGrowthStepLength);

        XfemCrack crackResult;
        auto crackPoint = model.nextPoint();
        crackResult.vCrackPoint = HVector2d(crackPoint.x(), crackPoint.y());
        crackResult.dK1 = model.K1();
        crackResult.dK2 = model.K2();
        m_vecCrackResult.push_back(crackResult);
    }

    return true;
}