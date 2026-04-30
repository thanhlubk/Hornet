#pragma once
#include "HESolveCrackPropagationCommon.h"

namespace XFEMCrackPropagation {

enum class HESolveCrackPropagationNodeClass {
    Standard,
    Heaviside,
    Asymptotic
};

std::string toString(HESolveCrackPropagationNodeClass value);

class HESolveCrackPropagationNode {
public:
    HESolveCrackPropagationNode(double x, double y, HESolveCrackPropagationNodeClass classify);
    virtual ~HESolveCrackPropagationNode() = default;

    const Vec2& coordinate() const noexcept { return coordinate_; }
    const Vec2& displacement() const noexcept { return displacement_; }
    const Vec4& stress() const noexcept { return stress_; }
    const std::vector<Vec4>& stressList() const noexcept { return stressList_; }
    std::vector<Vec4>& stressList() noexcept { return stressList_; }
    HESolveCrackPropagationNodeClass classify() const noexcept { return classify_; }

    virtual double levelSet() const noexcept { return 0.0; }
    virtual void setLevelSet(double) {}

    virtual const Vec2& nearestTip() const noexcept;
    virtual void setNearestTip(const Vec2&);

    virtual double tipAngle() const noexcept;
    virtual void setTipAngle(double);

    void createStress();
    void setDisplacement(const Vec2& value) { displacement_ = value; }

protected:
    Vec2 coordinate_;
    HESolveCrackPropagationNodeClass classify_;
    std::vector<Vec4> stressList_;
    Vec4 stress_ = Vec4::Zero();
    Vec2 displacement_ = Vec2::Zero();
};

class HESolveCrackPropagationStdNode final : public HESolveCrackPropagationNode {
public:
    HESolveCrackPropagationStdNode(double x, double y);
    double levelSet() const noexcept override { return levelSet_; }

private:
    double levelSet_ = 0.0;
};

class HESolveCrackPropagationHevNode final : public HESolveCrackPropagationNode {
public:
    HESolveCrackPropagationHevNode(double x, double y);
    double levelSet() const noexcept override { return levelSet_; }
    void setLevelSet(double value) override { levelSet_ = value; }

private:
    double levelSet_ = 0.0;
};

class HESolveCrackPropagationAsymptNode final : public HESolveCrackPropagationNode {
public:
    HESolveCrackPropagationAsymptNode(double x, double y);

    double levelSet() const noexcept override { return levelSet_; }
    void setLevelSet(double value) override { levelSet_ = value; }

    const Vec2& nearestTip() const noexcept override { return nearestTip_; }
    void setNearestTip(const Vec2& value) override { nearestTip_ = value; }

    double tipAngle() const noexcept override { return tipAngle_; }
    void setTipAngle(double value) override { tipAngle_ = value; }

private:
    Vec2 nearestTip_ = Vec2::Zero();
    double tipAngle_ = 0.0;
    double levelSet_ = 0.0;
};

using HESolveCrackPropagationNodePtr = std::shared_ptr<HESolveCrackPropagationNode>;

} // namespace XFEMCrackPropagation
