#pragma once

#include <cstdlib>
#include "Vector3.h"

namespace math
{

namespace detail
{
    // Delivers 10.0^significantDigits
    constexpr double RoundingFactor(std::size_t significantDigits)
    {
        return significantDigits == 1 ? 10.0 : 10.0 * RoundingFactor(significantDigits - 1);
    }
}

// A hash combination function based on the one used in boost and found on stackoverflow
inline void combineHash(std::size_t& seed, std::size_t hash)
{
    seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

inline std::size_t hashDouble(double value, std::size_t significantDigits)
{
    return static_cast<std::size_t>(value * detail::RoundingFactor(significantDigits));
}

inline std::size_t hashVector3(const Vector3& v, std::size_t significantDigits)
{
    auto xHash = math::hashDouble(v.x(), significantDigits);
    auto yHash = math::hashDouble(v.y(), significantDigits);
    auto zHash = math::hashDouble(v.z(), significantDigits);

    math::combineHash(xHash, yHash);
    math::combineHash(xHash, zHash);

    return xHash;
}

}
