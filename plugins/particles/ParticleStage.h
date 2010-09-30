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
 *
 * Most of the member descriptions are directly taken from the D3 SDK.
 */
class ParticleStage
{
	friend std::ostream& operator<< (std::ostream&, const ParticleStage&);
	
	// Number of particles
	int _count;						// total number of particles, although some may be invisible at a given time
	
	// Material to render onto each quad
	std::string _material;
	
	float _duration;				// Duration in seconds
	float _cycles;					// allows things to oneShot ( 1 cycle ) or run for a set number of cycles
									// on a per stage basis
	float _bunching;				// 0.0 = all come out at first instant, 1.0 = evenly spaced over cycle time

	float _timeOffset;				// time offset from system start for the first particle to spawn
	float _deadTime;				// time after particleLife before respawning

	int _cycleMsec;					// calculated as ( _duration + _deadTime ) in msec, read-only for public

	Vector4 _colour;				// Render colour
	Vector4 _fadeColour;			// either 0 0 0 0 for additive, or 1 1 1 0 for blended materials

	float _fadeInFraction;			// in 0.0 to 1.0 range
	float _fadeOutFraction;			// in 0.0 to 1.0 range
	float _fadeIndexFraction;		// in 0.0 to 1.0 range, causes later index smokes to be more faded 

	int	_animationFrames;			// if > 1, subdivide the texture S axis into frames and crossfade
	float _animationRate;			// frames per second
	
	float _initialAngle;			// in degrees, random angle is used if zero ( default ) 

	float _boundsExpansion;			// user tweak to fix poorly calculated bounds

	bool _randomDistribution;		// randomly orient the quad on emission ( defaults to true ) 
	bool _entityColor;				// force color from render entity ( fadeColor is still valid )

	float _gravity;					// can be negative to float up
	bool _applyWorldGravity;		// apply gravity in world space

	Vector3 _offset;				// offset from origin to spawn all particles, also applies to customPath

	/*
	This is an excerpt from the D3 SDK declparticle.h:

	//-------------------------------	// standard path parms
		
"distribution"	prtDistribution_t		distributionType;
"distribution"	float					distributionParms[4];
	
"direction"	prtDirection_t			directionType;
"direction"	float					directionParms[4];
	
"speed"	idParticleParm			speed;
	
	//------------------------------	// custom path will completely replace the standard path calculations
	
"customPath"	prtCustomPth_t			customPathType;		// use custom C code routines for determining the origin
"customPath"	float					customPathParms[8];
	
	//--------------------------------
	
"rotation"	idParticleParm			rotationSpeed;		// half the particles will have negative rotation speeds
	
"orientation"	prtOrientation_t		orientation;	// view, aimed, or axis fixed
"orientation" float					orientationParms[4];

"size"	idParticleParm			size;
"aspect"	idParticleParm			aspect;				// greater than 1 makes the T axis longer

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
	 * Get the shader name.
	 */
	const std::string& getMaterialName() const { return _material; }

	/** 
	 * Set the shader name.
	 */
	void setMaterialName(const std::string& material) { _material = material; }

	/** 
	 * Get the particle count.
	 */
	int getCount() const { return _count; }

	/** 
	 * Set the particle count.
	 */
	void setCount(int count) { _count = count; }

	/**
	 * Get the duration in seconds.
	 */
	float getDuration() const { return _duration; }

	/**
	 * Set the duration in seconds, updates cyclemsec.
	 */
	void setDuration(float duration) { _duration = duration; recalculateCycleMsec(); }

	/**
	 * Returns ( duration + deadTime ) in msec. 
	 */
	int getCycleMsec() const { return _cycleMsec; }

	/**
	 * Get the cycles value. 
	 */
	float getCycles() const { return _cycles; }

	/**
	 * Set the cycles value. 
	 */
	void setCycles(float cycles) { _cycles = clampZeroOrPositive(cycles); }

	/**
	 * Get the bunching value [0..1]
	 */
	float getBunching() const { return _bunching; }

	/**
	 * Set the bunching value [0..1]
	 */
	void setBunching(float value) { _bunching = clampOneZero(value); }

	/**
	 * Get the time offset in seconds
	 */
	float getTimeOffset() const { return _timeOffset; }

	/**
	 * Set the time offset in seconds
	 */
	void setTimeOffset(float value) { _timeOffset = value; }

	/**
	 * Get the dead time in seconds
	 */
	float getDeadTime() const { return _deadTime; }

	/**
	 * Set the dead time in seconds, updates cyclemsec.
	 */
	void setDeadTime(float value) { _deadTime = value; recalculateCycleMsec(); }

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

	/** 
	 * Get the animation frames.
	 */
	int getAnimationFrames() const { return _animationFrames; }

	/** 
	 * Set the animation frames.
	 */
	void setAnimationFrames(int animationFrames) { _animationFrames = animationFrames; }

	/** 
	 * Get the animation rate.
	 */
	float getAnimationRate() const { return _animationRate; }

	/** 
	 * Set the animation frames.
	 */
	void setAnimationRate(float animationRate) { _animationRate = animationRate; }

	/** 
	 * Get the initial angle.
	 */
	float getInitialAngle() const { return _initialAngle; }

	/** 
	 * Set the initial angle.
	 */
	void setInitialAngle(float angle) { _initialAngle = angle; }

	/** 
	 * Get the bounds expansion value.
	 */
	float getBoundsExpansion() const { return _boundsExpansion; }

	/** 
	 * Set the bounds expansion value.
	 */
	void setBoundsExpansion(float value) { _boundsExpansion = value; }

	/** 
	 * Get the random distribution flag.
	 */
	bool getRandomDistribution() const { return _randomDistribution; }

	/** 
	 * Set the random distribution flag.
	 */
	void setRandomDistribution(bool value) { _randomDistribution = value; }

	/** 
	 * Get the "use entity colour" flag.
	 */
	bool getUseEntityColour() const { return _entityColor; }

	/** 
	 * Set the "use entity colour" flag.
	 */
	void setUseEntityColour(bool value) { _entityColor = value; }

	/** 
	 * Get the gravity factor.
	 */
	float getGravity() const { return _gravity; }

	/** 
	 * Set the gravity factor.
	 */
	void setGravity(float value) { _gravity = value; }

	/** 
	 * Get the "apply gravity in world space" flag.
	 */
	bool getWorldGravityFlag() const { return _applyWorldGravity; }

	/** 
	 * Get the "apply gravity in world space" flag.
	 */
	void setWorldGravityFlag(bool value) { _applyWorldGravity = value; }

	/**
	 * Get the offset vector.
	 */
	const Vector3& getOffset() const { return _offset; }

	/**
	 * Set the offset vector.
	 */
	void setOffset(const Vector3& value) { _offset = value; }

	// Parser method, reads in all stage parameters from the given token stream
	// The initial opening brace { has already been parsed.
	// The routine will continue parsing until the matching closing } is encountered.
	void parseFromTokens(parser::DefTokeniser& tok);

private:
	void recalculateCycleMsec()
	{
		_cycleMsec = static_cast<int>(_duration + _deadTime) * 1000;
	}

	// Clamps the given float to the range [0..1]
	float clampOneZero(float input)
	{
		if (input < 0.0f) return 0.0f;
		if (input > 1.0f) return 1.0f;

		return input;
	}

	// Clamps the given float to positive values
	float clampZeroOrPositive(float input)
	{
		return (input < 0.0f) ? 0.0f : input;
	}

	Vector3 parseVector3(parser::DefTokeniser& tok);
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
