#pragma once

#include "math/Vector3.h"
#include "math/Vector4.h"
#include <boost/random/linear_congruential.hpp>

// greebo: Thanks for nothing to the boost.random developers who changed the class layout completely 
// between 1.46 and 1.47, without leaving a way to have code that is compatible with both versions,
// not even for a few months.
#if BOOST_VERSION >= 104700
#define GET_BOOST_RAND48_MAX(x) (boost::rand48::max())
#else
#define GET_BOOST_RAND48_MAX(x) (x.max())
#endif

namespace particles
{

/**
 * A structure holding info about how to draw a certain particle,
 * including texcoords, fade colour, etc.
 *
 * This info can apply to a single ParticleQuad or a whole quad group
 * if the particle stage is animated or aimed.
 */
struct ParticleRenderInfo
{
	std::size_t index;	// zero-based index of this particle within a stage

	float timeSecs;		// time in seconds
	float timeFraction;	// time fraction within particle lifetime

	Vector3 origin;
	Vector4 colour;		// resulting colour

	float angle;		// the angle of the quad
	float size;			// the desired size (might be overridden when aimed)
	float aspect;		// the desired aspect ratio (might be overridden when aimed)

	float sWidth;		// the horizontal amount of texture space occupied by this particle (for anims)
	float t0;			// Vertical texture coordinate
	float tWidth;		// the vertical amount of texture space occupied by this particle (for aiming)

	float rand[5];		// 5 random numbers needed for pathing

	std::size_t animFrames; // animation: number of frames (0 if not animated)
	std::size_t curFrame;	// animation: current frame
	std::size_t nextFrame;	// animation: next frame

	Vector4 curColour;
	Vector4 nextColour;

	ParticleRenderInfo() :
		index(0),
		angle(0),
		sWidth(1),
		t0(0),
		tWidth(1)
	{}

	// Construct a particle with the given index, and fill in the random variables
	ParticleRenderInfo(std::size_t index_, boost::rand48& random) :
		index(index_),
		angle(0),
		sWidth(1),
		t0(0),
		tWidth(1)
	{
		boost::rand48::result_type maxVal = GET_BOOST_RAND48_MAX(random);

		// Generate five random numbers for path calcs, this is needed in calculateOrigin
		rand[0] = static_cast<float>(random()) / maxVal;
		rand[1] = static_cast<float>(random()) / maxVal;
		rand[2] = static_cast<float>(random()) / maxVal;
		rand[3] = static_cast<float>(random()) / maxVal;
		rand[4] = static_cast<float>(random()) / maxVal;
	}
};

} // namespace
