#ifndef PARTICLESTAGE_H_
#define PARTICLESTAGE_H_

#include "math/Vector4.h"

#include <iostream>
#include "parser/DefTokeniser.h"

namespace particles
{

/* 
Excerpt from the D3 SDK:

class idParticleParm {
public:
							idParticleParm( void ) { table = NULL; from = to = 0.0f; }

	const idDeclTable *		table;
	float					from;
	float					to;
	
	float					Eval( float frac, idRandom &rand ) const;
	float					Integrate( float frac, idRandom &rand ) const;
};


typedef enum {
	PDIST_RECT,				// ( sizeX sizeY sizeZ )
	PDIST_CYLINDER,			// ( sizeX sizeY sizeZ )
	PDIST_SPHERE			// ( sizeX sizeY sizeZ ringFraction )
							// a ringFraction of zero allows the entire sphere, 0.9 would only
							// allow the outer 10% of the sphere
} prtDistribution_t;

typedef enum {
	PDIR_CONE,				// parm0 is the solid cone angle
	PDIR_OUTWARD			// direction is relative to offset from origin, parm0 is an upward bias
} prtDirection_t;

typedef enum {
	PPATH_STANDARD,
	PPATH_HELIX,			// ( sizeX sizeY sizeZ radialSpeed climbSpeed )
	PPATH_FLIES,
	PPATH_ORBIT,
	PPATH_DRIP
} prtCustomPth_t;

typedef enum {
	POR_VIEW,
	POR_AIMED,				// angle and aspect are disregarded
	POR_X,
	POR_Y,
	POR_Z
} prtOrientation_t;

typedef struct renderEntity_s renderEntity_t;
typedef struct renderView_s renderView_t;

typedef struct {
	const renderEntity_t *	renderEnt;			// for shaderParms, etc
	const renderView_t *	renderView;
	int						index;				// particle number in the system
	float					frac;				// 0.0 to 1.0
	idRandom				random;
	idVec3					origin;				// dynamic smoke particles can have individual origins and axis
	idMat3					axis;


	float					age;				// in seconds, calculated as fraction * stage->particleLife
	idRandom				originalRandom;		// needed so aimed particles can reset the random for another origin calculation
	float					animationFrameFrac;	// set by ParticleTexCoords, used to make the cross faded version
} particleGen_t;

*/

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
	
	float _duration;				// Duration in seconds
	
	Vector4 _colour;				// Render colour
	Vector4 _fadeColour;			// Fade colour

	float _fadeInFraction;			// in 0.0 to 1.0 range
	float _fadeOutFraction;			// in 0.0 to 1.0 range
	float _fadeIndexFraction;		// in 0.0 to 1.0 range, causes later index smokes to be more faded 

	/*
	This is an excerpt from the D3 SDK declparticle.h:

	const idMaterial *		material;

	int						totalParticles;		// total number of particles, although some may be invisible at a given time
	float					cycles;				// allows things to oneShot ( 1 cycle ) or run for a set number of cycles
												// on a per stage basis

	int						cycleMsec;			// ( particleLife + deadTime ) in msec

	float					spawnBunching;		// 0.0 = all come out at first instant, 1.0 = evenly spaced over cycle time
	float					particleLife;		// total seconds of life for each particle
	float					timeOffset;			// time offset from system start for the first particle to spawn
	float					deadTime;			// time after particleLife before respawning
	
	//-------------------------------	// standard path parms
		
	prtDistribution_t		distributionType;
	float					distributionParms[4];
	
	prtDirection_t			directionType;
	float					directionParms[4];
	
	idParticleParm			speed;
	float					gravity;				// can be negative to float up
	bool					worldGravity;			// apply gravity in world space
	bool					randomDistribution;		// randomly orient the quad on emission ( defaults to true ) 
	bool					entityColor;			// force color from render entity ( fadeColor is still valid )
	
	//------------------------------	// custom path will completely replace the standard path calculations
	
	prtCustomPth_t			customPathType;		// use custom C code routines for determining the origin
	float					customPathParms[8];
	
	//--------------------------------
	
	idVec3					offset;				// offset from origin to spawn all particles, also applies to customPath
	
	int						animationFrames;	// if > 1, subdivide the texture S axis into frames and crossfade
	float					animationRate;		// frames per second

	float					initialAngle;		// in degrees, random angle is used if zero ( default ) 
	idParticleParm			rotationSpeed;		// half the particles will have negative rotation speeds
	
	prtOrientation_t		orientation;	// view, aimed, or axis fixed
	float					orientationParms[4];

	idParticleParm			size;
	idParticleParm			aspect;				// greater than 1 makes the T axis longer

	idVec4					color;
	idVec4					fadeColor;			// either 0 0 0 0 for additive, or 1 1 1 0 for blended materials
	float					fadeInFraction;		// in 0.0 to 1.0 range
	float					fadeOutFraction;	// in 0.0 to 1.0 range
	float					fadeIndexFraction;	// in 0.0 to 1.0 range, causes later index smokes to be more faded 

	bool					hidden;				// for editor use
	//-----------------------------------

	float					boundsExpansion;	// user tweak to fix poorly calculated bounds

	idBounds				bounds;				// derived
	*/
	
public:
	// Create an empty particle stage with default values
	ParticleStage();

	// Create a particle stage from the given token stream
	ParticleStage(parser::DefTokeniser& tok);

	// Resets/clears all values to default. This is called by parseFromTokens().
	void reset();

	/** 
	 * Get the particle count.
	 */
	int getCount() const { return _count; }

	/** 
	 * Set the particle count.
	 */
	void setCount(int count) { _count = count; }
	
	/**
	 * Get the particle render colour.
	 */
	const Vector4& getColour() const { return _colour; }

	/**
	 * Set the particle render colour.
	 */
	void setColour(const Vector4& colour) { _colour = colour; }

	/**
	 * Get the particle render colour.
	 */
	const Vector4& getFadeColour() const { return _fadeColour; }

	/**
	 * Set the particle render colour.
	 */
	void setFadeColour(const Vector4& colour) { _fadeColour = colour; }
	
	/**
	 * Get the fade in fraction [0..1]
	 */
	float getFadeInFraction() const { return _fadeInFraction; }

	/**
	 * Set the fade in fraction [0..1]
	 */
	void setFadeInFraction(float fraction) { _fadeInFraction = clampOneZero(fraction); }

	/**
	 * Get the fade out fraction [0..1]
	 */
	float getFadeOutFraction() const { return _fadeOutFraction; }

	/**
	 * Set the fade out fraction [0..1]
	 */
	void setFadeOutFraction(float fraction) { _fadeOutFraction = clampOneZero(fraction); }

	/**
	 * Get the fade index fraction [0..1]
	 */
	float getFadeIndexFraction() const { return _fadeIndexFraction; }

	/**
	 * Set the fade index fraction [0..1]
	 */
	void setFadeIndexFraction(float fraction) { _fadeIndexFraction = clampOneZero(fraction); }

	// Parser method, reads in all stage parameters from the given token stream
	// The initial opening brace { has already been parsed.
	// The routine will continue parsing until the matching closing } is encountered.
	void parseFromTokens(parser::DefTokeniser& tok);

private:
	// Clamps the given float to the range [0..1]
	float clampOneZero(float input)
	{
		if (input < 0.0f) return 0.0f;
		if (input > 1.0f) return 1.0f;

		return input;
	}

	Vector4 parseVector4(parser::DefTokeniser& tok);
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
