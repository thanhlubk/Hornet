#pragma once
#include "HornetExecuteExport.h"
#include "HExecute.h"
#include <HornetUtil/HVector.h>
#include <eigen3/Eigen/Sparse>
#include <unordered_map>
#include <vector>

class HORNETEXECUTE_EXPORT HESolveXFEM2D : public HExecute
{
public:
    enum class XfemAnalysisType {
        Static,
        Modal
    };

    enum class XfemConditionType {
        PlaneStrain,
        PlaneStress
    };

    struct XfemCrack
    {
        HVector2d vCrackPoint;
        double dK1;
        double dK2;
    };

    HESolveXFEM2D(std::vector<std::vector<HVector2d>> vecCrack, double thickness, double density, double youngsModulus, double poissonRatio, XfemAnalysisType analysisType, XfemConditionType conditionType, double sifRadius, double growthStepLength, DatabaseSession* db, int step,bool transaction=true, bool logCommand=true);
    ~HESolveXFEM2D();

    std::vector<XfemCrack> getCrackResult() const noexcept { return m_vecCrackResult; }

protected:
    bool onExecute() override;
    void logCommand() override;

private:
    std::vector<std::vector<HVector2d>> m_vecCrack;
    double m_dThickness;
    double m_dDensity;
    double m_dYoungsModulus;
    double m_dPoissonRatio;
    XfemAnalysisType m_eAnalysisType;
    XfemConditionType m_eConditionType;
    double m_dSifRadius;
    double m_dGrowthStepLength;
    int m_iStep;
    std::vector<XfemCrack> m_vecCrackResult;
};
