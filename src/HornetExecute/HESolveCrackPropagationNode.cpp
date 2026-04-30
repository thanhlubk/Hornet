#include "HornetExecute/HESolveCrackPropagationNode.h"

namespace XFEMCrackPropagation {

std::string toString(HESolveCrackPropagationNodeClass value) {
    switch (value) {
        case HESolveCrackPropagationNodeClass::Standard: return "Standard";
        case HESolveCrackPropagationNodeClass::Heaviside: return "Heaviside";
        case HESolveCrackPropagationNodeClass::Asymptotic: return "Asymptotic";
    }
    return "Unknown";
}

HESolveCrackPropagationNode::HESolveCrackPropagationNode(double x, double y, HESolveCrackPropagationNodeClass classify)
    : coordinate_(x, y), classify_(classify) {}

const Vec2& HESolveCrackPropagationNode::nearestTip() const noexcept {
    static const Vec2 kZero = Vec2::Zero();
    return kZero;
}

void HESolveCrackPropagationNode::setNearestTip(const Vec2&) {}

double HESolveCrackPropagationNode::tipAngle() const noexcept {
    return 0.0;
}

void HESolveCrackPropagationNode::setTipAngle(double) {}

void HESolveCrackPropagationNode::createStress() {
    stress_.setZero();
    if (stressList_.empty()) {
        return;
    }
    for (const auto& item : stressList_) {
        stress_ += item;
    }
    stress_ /= static_cast<double>(stressList_.size());
}

HESolveCrackPropagationStdNode::HESolveCrackPropagationStdNode(double x, double y)
    : HESolveCrackPropagationNode(x, y, HESolveCrackPropagationNodeClass::Standard) {}

HESolveCrackPropagationHevNode::HESolveCrackPropagationHevNode(double x, double y)
    : HESolveCrackPropagationNode(x, y, HESolveCrackPropagationNodeClass::Heaviside) {}

HESolveCrackPropagationAsymptNode::HESolveCrackPropagationAsymptNode(double x, double y)
    : HESolveCrackPropagationNode(x, y, HESolveCrackPropagationNodeClass::Asymptotic) {}

} // namespace XFEMCrackPropagation
