#pragma once

#include "math/Vector3.h"

#include <cstdlib>
#include <string>

/**
 * Utility class containing a method to generate a random vector within a
 * certain distance of the world origin.
 */
class RandomOrigin
{
public:

    /**
     * Generate a random vector within <maxDist> of the world origin, returning
     * a string formatted correctly as an "origin" key.
     */
    static std::string generate(int maxDist)
    {
        // Generate three random numbers between 0 and maxDist
        float x = maxDist * (float(std::rand()) / float(RAND_MAX));
        float y = maxDist * (float(std::rand()) / float(RAND_MAX));
        float z = maxDist * (float(std::rand()) / float(RAND_MAX));

        // Construct a vector and return the formatted string
        return string::to_string(Vector3(x, y, z));
    }
};
