// HornetBase/Settings.h
#pragma once
#include <cstdint>
#include "HornetBaseExport.h"

enum class UnitSystem : std::uint8_t {
    SI = 0,
    Imperial
};

class HORNETBASE_EXPORT Setting
{
public:
    Setting() = default;

    UnitSystem unitSystem() const noexcept { return m_unitSystem; }
    void setUnitSystem(UnitSystem u) noexcept { m_unitSystem = u; }

    // add other core settings as needed
    // double lengthTolerance() const noexcept { ... }
    // void setLengthTolerance(double t) noexcept { ... }

private:
    UnitSystem m_unitSystem = UnitSystem::SI;
};
