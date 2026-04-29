#include "HornetExecute/HESolverXFEM2DNode.h"

namespace xfem {

std::string toString(HESolverXFEM2DNodeClass value) {
    switch (value) {
        case HESolverXFEM2DNodeClass::Standard: return "Standard";
        case HESolverXFEM2DNodeClass::Heaviside: return "Heaviside";
        case HESolverXFEM2DNodeClass::Asymptotic: return "Asymptotic";
    }
    return "Unknown";
}

HESolverXFEM2DNode::HESolverXFEM2DNode(double x, double y, HESolverXFEM2DNodeClass classify)
    : coordinate_(x, y), classify_(classify) {}

const Vec2& HESolverXFEM2DNode::nearestTip() const noexcept {
    static const Vec2 kZero = Vec2::Zero();
    return kZero;
}

void HESolverXFEM2DNode::setNearestTip(const Vec2&) {}

double HESolverXFEM2DNode::tipAngle() const noexcept {
    return 0.0;
}

void HESolverXFEM2DNode::setTipAngle(double) {}

void HESolverXFEM2DNode::createStress() {
    stress_.setZero();
    if (stressList_.empty()) {
        return;
    }
    for (const auto& item : stressList_) {
        stress_ += item;
    }
    stress_ /= static_cast<double>(stressList_.size());
}

HESolverXFEM2DStdNode::HESolverXFEM2DStdNode(double x, double y)
    : HESolverXFEM2DNode(x, y, HESolverXFEM2DNodeClass::Standard) {}

HESolverXFEM2DHevNode::HESolverXFEM2DHevNode(double x, double y)
    : HESolverXFEM2DNode(x, y, HESolverXFEM2DNodeClass::Heaviside) {}

HESolverXFEM2DAsymptNode::HESolverXFEM2DAsymptNode(double x, double y)
    : HESolverXFEM2DNode(x, y, HESolverXFEM2DNodeClass::Asymptotic) {}

} // namespace xfem
