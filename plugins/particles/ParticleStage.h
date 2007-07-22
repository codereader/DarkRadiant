#ifndef PARTICLESTAGE_H_
#define PARTICLESTAGE_H_

#include "math/Vector4.h"

#include <iostream>

namespace particles
{

/**
 * Representation of a single particle stage. Each stage consists of a set of
 * particles with the same properties (texture, acceleration etc).
 */
class ParticleStage
{
	friend std::ostream& operator<< (std::ostream&, const ParticleStage&);
	
	// Number of particles
	int _count;
	
	// Material to render onto each quad
	std::string _material;
	
	// Duration in seconds
	float _duration;
	
	// Render colour
	Vector4 _colour;
	
	// Fade colour
	Vector4 _fadeColour;
	
public:

	/** 
	 * Set the particle count.
	 */
	void setCount(int count) { _count = count; }
	
	/**
	 * Set the particle render colour.
	 */
	void setColour(const Vector4& colour) { _colour = colour; }
	
};

/**
 * Stream insertion operator for debugging.
 */
inline std::ostream& operator<< (std::ostream& os, const ParticleStage& stage) {
	os << "ParticleStage { count = " << stage._count << ", "
	   << "colour = " << stage._colour 
	   << " }";
	return os;
}

}

#endif /*PARTICLESTAGE_H_*/
