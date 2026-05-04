#include "HornetExecute/HESolveLinearAnalysis.h"
#include "HornetExecute/HESolveLinearAnalysisModel.h"
#include "HornetExecute/HESolveLinearAnalysisCommon.h"

HESolveLinearAnalysis::HESolveLinearAnalysis(double density, double youngsModulus, double poissonRatio, HESolve::AnalysisType analysisType, DatabaseSession* db, bool transaction, bool logCommand)
    : HExecute(db, transaction, logCommand),
    m_dDensity(density),
    m_dYoungsModulus(youngsModulus),
    m_dPoissonRatio(poissonRatio),
    m_eAnalysisType(analysisType),
    m_iNumModes(5) // default number of modes for modal analysis
{
}

HESolveLinearAnalysis::~HESolveLinearAnalysis()
{
}

void HESolveLinearAnalysis::logCommand()
{
    return;
}

bool HESolveLinearAnalysis::onExecute()
{
    auto model = FEMLinearStatic::HESolveLinearAnalysisModel(m_pDb, m_dYoungsModulus, m_dPoissonRatio, m_dDensity);
    model.createStiffnessMatrix();
    model.createMassMatrix();

    if (m_eAnalysisType == HESolve::AnalysisType::Modal)
        model.solve(HESolve::AnalysisType::Modal, static_cast<int>(m_iNumModes));
    else if (m_eAnalysisType == HESolve::AnalysisType::Static)
    {
        model.solve(HESolve::AnalysisType::Static);
    }

    if (m_eAnalysisType == HESolve::AnalysisType::Static)
    {
        model.createDisplacement();
        model.createStress();

        m_pDb->emplace<HIResult>(1);
        auto pResult = m_pDb->checkOut<HIResult>(1);
        if (!pResult)
            return false;

        pResult->setAnalysisType(HIResultAnalysisType::LinearStatic);
        pResult->setStep(1);
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
    else if (m_eAnalysisType == HESolve::AnalysisType::Modal)
    {
        auto vecFrequencies = model.frequency();
        for (auto i = 0; i < vecFrequencies.size(); i++)
        {
            model.createDisplacement(i);
            model.createStress();

            m_pDb->emplace<HIResult>(i + 1);
            auto pResult = m_pDb->checkOut<HIResult>(i + 1);
            if (!pResult)
                continue;

            pResult->setAnalysisType(HIResultAnalysisType::Modal);
            pResult->setStep(i + 1);
            auto frequency = vecFrequencies[i];
            pResult->setModalFrequency(frequency);

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
    }
    
    return true;
}