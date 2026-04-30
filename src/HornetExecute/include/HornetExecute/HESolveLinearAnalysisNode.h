#pragma once
#include "HESolveLinearAnalysisCommon.h"

namespace FEMLinearStatic {

class HESolveLinearAnalysisNode {
public:
    HESolveLinearAnalysisNode(double x, double y, double z);
    virtual ~HESolveLinearAnalysisNode() = default;

    const Vec3& coordinate() const noexcept { return coordinate_; }
    const Vec3& displacement() const noexcept { return displacement_; }
    const Vec7& stress() const noexcept { return stress_; }

    const std::vector<Vec7>& stressList() const noexcept { return stressList_; }
    std::vector<Vec7>& stressList() noexcept { return stressList_; }

    void createStress();
    void setDisplacement(const Vec3& value) { displacement_ = value; }
private:
    Vec3 coordinate_;
    std::vector<Vec7> stressList_;
    Vec7 stress_ = Vec7::Zero();
    Vec3 displacement_ = Vec3::Zero();
};

using HESolveLinearAnalysisNodePtr = std::shared_ptr<HESolveLinearAnalysisNode>;

} // namespace FEMLinearStatic
