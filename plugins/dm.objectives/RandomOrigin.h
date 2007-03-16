#ifndef RANDOMORIGIN_H_
#define RANDOMORIGIN_H_

#include "math/Vector3.h"

#include <cstdlib>
#include <string>

namespace objectives
{

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
	static std::string generate(int maxDist) {
		
		// Generate three random numbers between 0 and maxDist
		int x = int(maxDist * (float(std::rand()) / float(RAND_MAX)));
		int y = int(maxDist * (float(std::rand()) / float(RAND_MAX)));
		int z = int(maxDist * (float(std::rand()) / float(RAND_MAX)));
		
		// Construct a vector and return the formatted string
		return Vector3(x, y, z);
	}
};

}

#endif /*RANDOMORIGIN_H_*/
