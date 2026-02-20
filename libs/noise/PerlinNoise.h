#pragma once

#include <cmath>
#include <cstdint>
#include <array>

namespace noise
{

class PerlinNoise
{
private:
    std::array<int, 512> _perm;

    static double fade(double t)
    {
        return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
    }

    static double lerp(double t, double a, double b)
    {
        return a + t * (b - a);
    }

    static double grad(int hash, double x, double y, double z)
    {
        int h = hash & 15;
        double u = h < 8 ? x : y;
        double v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

    static double grad2D(int hash, double x, double y)
    {
        int h = hash & 7;
        double u = h < 4 ? x : y;
        double v = h < 4 ? y : x;
        return ((h & 1) != 0 ? -u : u) + ((h & 2) != 0 ? -2.0 * v : 2.0 * v);
    }

public:
    explicit PerlinNoise(unsigned int seed = 0)
    {
        std::array<int, 256> p;
        for (int i = 0; i < 256; ++i)
        {
            p[i] = i;
        }

        uint32_t state = seed;
        auto nextRand = [&state]() -> uint32_t
        {
            state = state * 1103515245 + 12345;
            return (state >> 16) & 0x7FFF;
        };

        for (int i = 255; i > 0; --i)
        {
            int j = nextRand() % (i + 1);
            std::swap(p[i], p[j]);
        }

        for (int i = 0; i < 256; ++i)
        {
            _perm[i] = p[i];
            _perm[256 + i] = p[i];
        }
    }

    double noise2D(double x, double y) const
    {
        int X = static_cast<int>(std::floor(x)) & 255;
        int Y = static_cast<int>(std::floor(y)) & 255;

        x -= std::floor(x);
        y -= std::floor(y);

        double u = fade(x);
        double v = fade(y);

        int A = _perm[X] + Y;
        int AA = _perm[A];
        int AB = _perm[A + 1];
        int B = _perm[X + 1] + Y;
        int BA = _perm[B];
        int BB = _perm[B + 1];

        double res = lerp(v,
                          lerp(u, grad2D(_perm[AA], x, y), grad2D(_perm[BA], x - 1, y)),
                          lerp(u, grad2D(_perm[AB], x, y - 1), grad2D(_perm[BB], x - 1, y - 1)));

        return res * 0.5;
    }

    double noise3D(double x, double y, double z) const
    {
        int X = static_cast<int>(std::floor(x)) & 255;
        int Y = static_cast<int>(std::floor(y)) & 255;
        int Z = static_cast<int>(std::floor(z)) & 255;

        x -= std::floor(x);
        y -= std::floor(y);
        z -= std::floor(z);

        double u = fade(x);
        double v = fade(y);
        double w = fade(z);

        int A = _perm[X] + Y;
        int AA = _perm[A] + Z;
        int AB = _perm[A + 1] + Z;
        int B = _perm[X + 1] + Y;
        int BA = _perm[B] + Z;
        int BB = _perm[B + 1] + Z;

        return lerp(
            w,
            lerp(v,
                 lerp(u, grad(_perm[AA], x, y, z), grad(_perm[BA], x - 1, y, z)),
                 lerp(u, grad(_perm[AB], x, y - 1, z), grad(_perm[BB], x - 1, y - 1, z))),
            lerp(v,
                 lerp(u, grad(_perm[AA + 1], x, y, z - 1), grad(_perm[BA + 1], x - 1, y, z - 1)),
                 lerp(u,
                      grad(_perm[AB + 1], x, y - 1, z - 1),
                      grad(_perm[BB + 1], x - 1, y - 1, z - 1))));
    }
};

} // namespace noise
