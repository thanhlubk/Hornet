#include "HornetExecute/HESolveLinearAnalysisNode.h"

namespace FEMLinearStatic {

HESolveLinearAnalysisNode::HESolveLinearAnalysisNode(double x, double y, double z)
    : coordinate_(x, y, z) {}

void HESolveLinearAnalysisNode::createStress() {
    stress_.setZero();
    if (stressList_.empty()) {
        return;
    }
    for (const auto& item : stressList_) {
        stress_ += item;
    }
    stress_ /= static_cast<double>(stressList_.size());
}

} // namespace FEMLinearStatic
