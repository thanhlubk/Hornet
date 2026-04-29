#pragma once
#include "HESolverXFEM2DCommon.h"

namespace xfem {

enum class HESolverXFEM2DNodeClass {
    Standard,
    Heaviside,
    Asymptotic
};

std::string toString(HESolverXFEM2DNodeClass value);

class HESolverXFEM2DNode {
public:
    HESolverXFEM2DNode(double x, double y, HESolverXFEM2DNodeClass classify);
    virtual ~HESolverXFEM2DNode() = default;

    const Vec2& coordinate() const noexcept { return coordinate_; }
    const Vec2& displacement() const noexcept { return displacement_; }
    const Vec4& stress() const noexcept { return stress_; }
    const std::vector<Vec4>& stressList() const noexcept { return stressList_; }
    std::vector<Vec4>& stressList() noexcept { return stressList_; }
    HESolverXFEM2DNodeClass classify() const noexcept { return classify_; }

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
    HESolverXFEM2DNodeClass classify_;
    std::vector<Vec4> stressList_;
    Vec4 stress_ = Vec4::Zero();
    Vec2 displacement_ = Vec2::Zero();
};

class HESolverXFEM2DStdNode final : public HESolverXFEM2DNode {
public:
    HESolverXFEM2DStdNode(double x, double y);
    double levelSet() const noexcept override { return levelSet_; }

private:
    double levelSet_ = 0.0;
};

class HESolverXFEM2DHevNode final : public HESolverXFEM2DNode {
public:
    HESolverXFEM2DHevNode(double x, double y);
    double levelSet() const noexcept override { return levelSet_; }
    void setLevelSet(double value) override { levelSet_ = value; }

private:
    double levelSet_ = 0.0;
};

class HESolverXFEM2DAsymptNode final : public HESolverXFEM2DNode {
public:
    HESolverXFEM2DAsymptNode(double x, double y);

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

using HESolverXFEM2DNodePtr = std::shared_ptr<HESolverXFEM2DNode>;

} // namespace xfem
