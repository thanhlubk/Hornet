#pragma once
#include "HornetExecuteExport.h"
#include "HExecute.h"
#include "HESolveDef.h"

class HORNETEXECUTE_EXPORT HESolveLinearAnalysis : public HExecute
{
public:
    HESolveLinearAnalysis(double density, double youngsModulus, double poissonRatio, HESolve::AnalysisType analysisType, DatabaseSession* db, bool transaction=true, bool logCommand=true);
    ~HESolveLinearAnalysis();

    void setModalAnalysisNumModes(std::uint64_t numModes) { m_iNumModes = numModes; }

protected:
    bool onExecute() override;
    void logCommand() override;

private:
    double m_dThickness;
    double m_dDensity;
    double m_dYoungsModulus;
    double m_dPoissonRatio;
    HESolve::AnalysisType m_eAnalysisType;

    std::uint64_t m_iNumModes; // for modal analysis
};
