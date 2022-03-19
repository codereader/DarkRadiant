#pragma once

#include <cstdlib>
#include <memory>
#include "Vector3.h"
#include "SHA256.h"

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

template<typename ElementType>
inline std::size_t hashVector3(const BasicVector3<ElementType>& v, std::size_t significantDigits)
{
    auto xHash = math::hashDouble(v.x(), significantDigits);
    auto yHash = math::hashDouble(v.y(), significantDigits);
    auto zHash = math::hashDouble(v.z(), significantDigits);

    math::combineHash(xHash, yHash);
    math::combineHash(xHash, zHash);

    return xHash;
}

// Convenience wrapper around the C-style functions in the SHA256.h header
class Hash
{
private:
    std::unique_ptr<SHA256_CTX> _context;

public:
    Hash() :
        _context(new SHA256_CTX)
    {
        sha256_init(_context.get());
    }

    void addSizet(std::size_t value)
    {
        sha256_update(_context.get(), reinterpret_cast<const uint8_t*>(&value), sizeof(value));
    }

    void addDouble(double value, std::size_t significantDigits)
    {
        auto intValue = static_cast<std::size_t>(value * detail::RoundingFactor(significantDigits));
        addSizet(intValue);
    }

    template<typename ElementType>
    void addVector3(const BasicVector3<ElementType>& v, std::size_t significantDigits)
    {
        std::size_t components[3] =
        {
            static_cast<std::size_t>(v.x() * detail::RoundingFactor(significantDigits)),
            static_cast<std::size_t>(v.y() * detail::RoundingFactor(significantDigits)),
            static_cast<std::size_t>(v.z() * detail::RoundingFactor(significantDigits)),
        };
        
        sha256_update(_context.get(), reinterpret_cast<const uint8_t*>(&components), sizeof(components));
    }

    void addString(const std::string& str)
    {
        if (str.length() == 0) return;

        sha256_update(_context.get(), reinterpret_cast<const uint8_t*>(str.data()), str.length());
    }

    operator std::string() const
    {
        uint8_t digest[SHA256_BLOCK_SIZE];
        sha256_final(_context.get(), digest);

        constexpr char hexChars[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

        std::string hexString(sizeof(digest) * 2, '\0');

        for (auto i = 0; i < sizeof(digest); ++i)
        {
            hexString[i*2] = hexChars[(digest[i] & 0xF0) >> 4];
            hexString[i*2 + 1] = hexChars[digest[i] & 0x0F];
        }

        return hexString;
    }
};

}
