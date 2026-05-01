#pragma once
#include "HornetExecuteExport.h"
#include "HExecute.h"
#include "HESolveDef.h"
#include <HornetUtil/HVector.h>
#include <eigen3/Eigen/Sparse>
#include <unordered_map>
#include <vector>

class HORNETEXECUTE_EXPORT HESolveCrackPropagation : public HExecute
{
public:
    struct XfemCrack
    {
        HVector2d vCrackPoint;
        double dK1;
        double dK2;
    };

    HESolveCrackPropagation(std::vector<std::vector<HVector2d>> vecCrack, double thickness, double density, double youngsModulus, double poissonRatio, HESolve::AnalysisType analysisType, HESolve::ConditionType conditionType, double sifRadius, double growthStepLength, DatabaseSession* db, int step,bool transaction=true, bool logCommand=true);
    ~HESolveCrackPropagation();

    std::vector<XfemCrack> getCrackResult() const noexcept { return m_vecCrackResult; }
    std::vector<std::vector<HVector2d>> getCrack() const noexcept { return m_vecCrack; }
protected:
    bool onExecute() override;
    void logCommand() override;
    void extendCrack(const HVector2d& crackTip, const HVector2d& nextPoint);

private:
    std::vector<std::vector<HVector2d>> m_vecCrack;
    double m_dThickness;
    double m_dDensity;
    double m_dYoungsModulus;
    double m_dPoissonRatio;
    HESolve::AnalysisType m_eAnalysisType;
    HESolve::ConditionType m_eConditionType;
    double m_dSifRadius;
    double m_dGrowthStepLength;
    int m_iStep;
    std::vector<XfemCrack> m_vecCrackResult;
};
