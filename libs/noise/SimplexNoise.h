#pragma once

#include <cmath>
#include <cstdint>
#include <array>

namespace noise
{

class SimplexNoise
{
private:
    std::array<int, 512> _perm;
    std::array<int, 512> _permMod12;

    static constexpr double grad2[][2] = {
        {1, 1}, {-1, 1}, {1, -1}, {-1, -1}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}};

    static constexpr double grad3[][3] = {{1, 1, 0},
                                          {-1, 1, 0},
                                          {1, -1, 0},
                                          {-1, -1, 0},
                                          {1, 0, 1},
                                          {-1, 0, 1},
                                          {1, 0, -1},
                                          {-1, 0, -1},
                                          {0, 1, 1},
                                          {0, -1, 1},
                                          {0, 1, -1},
                                          {0, -1, -1}};

    static constexpr double F2 = 0.5 * (std::sqrt(3.0) - 1.0);
    static constexpr double G2 = (3.0 - std::sqrt(3.0)) / 6.0;

    static double dot2(const double g[], double x, double y)
    {
        return g[0] * x + g[1] * y;
    }

    static double dot3(const double g[], double x, double y, double z)
    {
        return g[0] * x + g[1] * y + g[2] * z;
    }

public:
    explicit SimplexNoise(unsigned int seed = 0)
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
            _permMod12[i] = p[i] % 12;
            _permMod12[256 + i] = p[i] % 12;
        }
    }

    double noise2D(double x, double y) const
    {
        double n0, n1, n2;

        double s = (x + y) * F2;
        int i = static_cast<int>(std::floor(x + s));
        int j = static_cast<int>(std::floor(y + s));

        double t = (i + j) * G2;
        double X0 = i - t;
        double Y0 = j - t;
        double x0 = x - X0;
        double y0 = y - Y0;

        int i1, j1;
        if (x0 > y0)
        {
            i1 = 1;
            j1 = 0;
        }
        else
        {
            i1 = 0;
            j1 = 1;
        }

        double x1 = x0 - i1 + G2;
        double y1 = y0 - j1 + G2;
        double x2 = x0 - 1.0 + 2.0 * G2;
        double y2 = y0 - 1.0 + 2.0 * G2;

        int ii = i & 255;
        int jj = j & 255;
        int gi0 = _perm[ii + _perm[jj]] & 7;
        int gi1 = _perm[ii + i1 + _perm[jj + j1]] & 7;
        int gi2 = _perm[ii + 1 + _perm[jj + 1]] & 7;

        double t0 = 0.5 - x0 * x0 - y0 * y0;
        if (t0 < 0)
        {
            n0 = 0.0;
        }
        else
        {
            t0 *= t0;
            n0 = t0 * t0 * dot2(grad2[gi0], x0, y0);
        }

        double t1 = 0.5 - x1 * x1 - y1 * y1;
        if (t1 < 0)
        {
            n1 = 0.0;
        }
        else
        {
            t1 *= t1;
            n1 = t1 * t1 * dot2(grad2[gi1], x1, y1);
        }

        double t2 = 0.5 - x2 * x2 - y2 * y2;
        if (t2 < 0)
        {
            n2 = 0.0;
        }
        else
        {
            t2 *= t2;
            n2 = t2 * t2 * dot2(grad2[gi2], x2, y2);
        }

        // Add contributions and scale to [-1, 1]
        return 70.0 * (n0 + n1 + n2);
    }

    double noise3D(double x, double y, double z) const
    {
        static constexpr double F3 = 1.0 / 3.0;
        static constexpr double G3 = 1.0 / 6.0;

        double n0, n1, n2, n3;

        double s = (x + y + z) * F3;
        int i = static_cast<int>(std::floor(x + s));
        int j = static_cast<int>(std::floor(y + s));
        int k = static_cast<int>(std::floor(z + s));

        double t = (i + j + k) * G3;
        double X0 = i - t;
        double Y0 = j - t;
        double Z0 = k - t;
        double x0 = x - X0;
        double y0 = y - Y0;
        double z0 = z - Z0;

        int i1, j1, k1, i2, j2, k2;
        if (x0 >= y0)
        {
            if (y0 >= z0)
            {
                i1 = 1;
                j1 = 0;
                k1 = 0;
                i2 = 1;
                j2 = 1;
                k2 = 0;
            }
            else if (x0 >= z0)
            {
                i1 = 1;
                j1 = 0;
                k1 = 0;
                i2 = 1;
                j2 = 0;
                k2 = 1;
            }
            else
            {
                i1 = 0;
                j1 = 0;
                k1 = 1;
                i2 = 1;
                j2 = 0;
                k2 = 1;
            }
        }
        else
        {
            if (y0 < z0)
            {
                i1 = 0;
                j1 = 0;
                k1 = 1;
                i2 = 0;
                j2 = 1;
                k2 = 1;
            }
            else if (x0 < z0)
            {
                i1 = 0;
                j1 = 1;
                k1 = 0;
                i2 = 0;
                j2 = 1;
                k2 = 1;
            }
            else
            {
                i1 = 0;
                j1 = 1;
                k1 = 0;
                i2 = 1;
                j2 = 1;
                k2 = 0;
            }
        }

        double x1 = x0 - i1 + G3;
        double y1 = y0 - j1 + G3;
        double z1 = z0 - k1 + G3;
        double x2 = x0 - i2 + 2.0 * G3;
        double y2 = y0 - j2 + 2.0 * G3;
        double z2 = z0 - k2 + 2.0 * G3;
        double x3 = x0 - 1.0 + 3.0 * G3;
        double y3 = y0 - 1.0 + 3.0 * G3;
        double z3 = z0 - 1.0 + 3.0 * G3;

        int ii = i & 255;
        int jj = j & 255;
        int kk = k & 255;
        int gi0 = _permMod12[ii + _perm[jj + _perm[kk]]];
        int gi1 = _permMod12[ii + i1 + _perm[jj + j1 + _perm[kk + k1]]];
        int gi2 = _permMod12[ii + i2 + _perm[jj + j2 + _perm[kk + k2]]];
        int gi3 = _permMod12[ii + 1 + _perm[jj + 1 + _perm[kk + 1]]];

        double t0 = 0.6 - x0 * x0 - y0 * y0 - z0 * z0;
        if (t0 < 0)
        {
            n0 = 0.0;
        }
        else
        {
            t0 *= t0;
            n0 = t0 * t0 * dot3(grad3[gi0], x0, y0, z0);
        }

        double t1 = 0.6 - x1 * x1 - y1 * y1 - z1 * z1;
        if (t1 < 0)
        {
            n1 = 0.0;
        }
        else
        {
            t1 *= t1;
            n1 = t1 * t1 * dot3(grad3[gi1], x1, y1, z1);
        }

        double t2 = 0.6 - x2 * x2 - y2 * y2 - z2 * z2;
        if (t2 < 0)
        {
            n2 = 0.0;
        }
        else
        {
            t2 *= t2;
            n2 = t2 * t2 * dot3(grad3[gi2], x2, y2, z2);
        }

        double t3 = 0.6 - x3 * x3 - y3 * y3 - z3 * z3;
        if (t3 < 0)
        {
            n3 = 0.0;
        }
        else
        {
            t3 *= t3;
            n3 = t3 * t3 * dot3(grad3[gi3], x3, y3, z3);
        }

        return 32.0 * (n0 + n1 + n2 + n3);
    }
};

constexpr double SimplexNoise::grad2[][2];
constexpr double SimplexNoise::grad3[][3];

} // namespace noise
