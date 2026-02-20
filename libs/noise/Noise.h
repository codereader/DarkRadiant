#pragma once

#include "PerlinNoise.h"
#include "SimplexNoise.h"
#include <cmath>
#include <algorithm>

namespace noise
{

enum class Algorithm
{
    Perlin,
    Simplex,
    FBm,
    RidgedMultifractal
};

struct NoiseParameters
{
    Algorithm algorithm = Algorithm::Perlin;
    unsigned int seed = 0;
    double frequency = 1.0;   // Base frequency/scale
    double amplitude = 1.0;   // Output scale
    int octaves = 4;          // Number of layers (for fractal types)
    double persistence = 0.5; // Amplitude multiplier per octave
    double lacunarity = 2.0;  // Frequency multiplier per octave
    double offset = 1.0;      // Ridge offset (for ridged multifractal)
};

class NoiseGenerator
{
private:
    PerlinNoise _perlin;
    SimplexNoise _simplex;
    NoiseParameters _params;

    double baseNoise(double x, double y) const
    {
        switch (_params.algorithm)
        {
        case Algorithm::Perlin:
        case Algorithm::FBm:
        case Algorithm::RidgedMultifractal:
            return _perlin.noise2D(x, y);
        case Algorithm::Simplex:
            return _simplex.noise2D(x, y);
        default:
            return _perlin.noise2D(x, y);
        }
    }

    // Fractional Brownian Motion
    double fBm(double x, double y) const
    {
        double total = 0.0;
        double amplitude = 1.0;
        double frequency = 1.0;
        double maxValue = 0.0;

        for (int i = 0; i < _params.octaves; ++i)
        {
            total += _perlin.noise2D(x * frequency, y * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= _params.persistence;
            frequency *= _params.lacunarity;
        }

        return total / maxValue; // Normalize
    }

    // Simplex fBm variant
    double simplexFBm(double x, double y) const
    {
        double total = 0.0;
        double amplitude = 1.0;
        double frequency = 1.0;
        double maxValue = 0.0;

        for (int i = 0; i < _params.octaves; ++i)
        {
            total += _simplex.noise2D(x * frequency, y * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= _params.persistence;
            frequency *= _params.lacunarity;
        }

        return total / maxValue;
    }

    // Ridged Multifractal noise
    double ridgedMultifractal(double x, double y) const
    {
        double total = 0.0;
        double frequency = 1.0;
        double amplitude = 1.0;
        double weight = 1.0;

        for (int i = 0; i < _params.octaves; ++i)
        {
            double signal = _perlin.noise2D(x * frequency, y * frequency);
            signal = _params.offset - std::abs(signal);
            signal *= signal;

            signal *= weight;
            weight = std::clamp(signal * 2.0, 0.0, 1.0);

            total += signal * amplitude;
            frequency *= _params.lacunarity;
            amplitude *= _params.persistence;
        }

        return (total * 1.25) - 1.0;
    }

public:
    explicit NoiseGenerator(const NoiseParameters& params = NoiseParameters())
        : _perlin(params.seed), _simplex(params.seed), _params(params)
    {
    }

    void setParameters(const NoiseParameters& params)
    {
        if (params.seed != _params.seed)
        {
            _perlin = PerlinNoise(params.seed);
            _simplex = SimplexNoise(params.seed);
        }
        _params = params;
    }

    const NoiseParameters& getParameters() const
    {
        return _params;
    }

    double sample(double x, double y) const
    {
        double fx = x * _params.frequency;
        double fy = y * _params.frequency;

        double value = 0.0;

        switch (_params.algorithm)
        {
        case Algorithm::Perlin:
            value = _perlin.noise2D(fx, fy);
            break;

        case Algorithm::Simplex:
            value = _simplex.noise2D(fx, fy);
            break;

        case Algorithm::FBm:
            value = fBm(fx, fy);
            break;

        case Algorithm::RidgedMultifractal:
            value = ridgedMultifractal(fx, fy);
            break;
        }

        return value * _params.amplitude;
    }

    double sampleNormalized(double x, double y) const
    {
        double value = sample(x, y) / _params.amplitude;

        return (value + 1.0) * 0.5;
    }
};

} // namespace noise
