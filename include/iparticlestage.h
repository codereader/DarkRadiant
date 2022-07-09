#pragma once

#include <string>
#include <memory>

template<typename Element> class BasicVector3;
typedef BasicVector3<double> Vector3;
template<typename Element> class BasicVector4;
typedef BasicVector4<double> Vector4;

namespace particles
{

/**
 * greebo: A particle parameter represents a bounded member value
 * of a particle stage (e.g. speed or size).
 *
 * Use the evaluate() method to retrieve a particular value.
 *
 * It is modeled after the idParticleParam class in the D3 SDK.
 */
class IParticleParameter
{
public:
	// Get the lower bound of this parameter
	virtual float getFrom() const = 0;

	// Get the upper bound of this parameter
	virtual float getTo() const = 0;

	// Set the lower bound of this parameter
	virtual void setFrom(float value) = 0;

	// Set the upper bound of this parameter
	virtual void setTo(float value) = 0;

	// Return a specific value. Fraction == 0 returns the lower bound value.
	virtual float evaluate(float fraction) const = 0;

	// Returns the integrated value at the point <fraction>.
	virtual float integrate(float fraction) const = 0;

	// Comparison operators - particle parameters are considered equal 
	// if both from and to are equal
	virtual bool operator==(const IParticleParameter& other) const = 0;
	virtual bool operator!=(const IParticleParameter& other) const = 0;
};

/**
 * \brief
 * Definition of a single particle stage.
 *
 * Each stage consists of a set of particles with the same properties (texture,
 * acceleration etc).
 *
 * Most of the member descriptions are directly taken from the D3 SDK.
 */
class IStageDef
{
public:
    using Ptr = std::shared_ptr<IStageDef>;

	// Particle orientation
	enum OrientationType
	{
		ORIENTATION_VIEW,
		ORIENTATION_AIMED,	// angle and aspect are disregarded
		ORIENTATION_X,
		ORIENTATION_Y,
		ORIENTATION_Z
	};

	// Particle distribution
	enum DistributionType
	{
		DISTRIBUTION_RECT,		// ( sizeX sizeY sizeZ )
		DISTRIBUTION_CYLINDER,	// ( sizeX sizeY sizeZ )
		DISTRIBUTION_SPHERE		// ( sizeX sizeY sizeZ ringFraction )
								// a ringFraction of zero allows the entire sphere, 0.9 would only
								// allow the outer 10% of the sphere
	};

	// Particle direction
	enum DirectionType
	{
		DIRECTION_CONE,		// parm0 is the solid cone angle
		DIRECTION_OUTWARD	// direction is relative to offset from origin, parm0 is an upward bias
	};

	enum CustomPathType
	{
		PATH_STANDARD,
		PATH_HELIX,		// ( sizeX sizeY sizeZ radialSpeed climbSpeed )
		PATH_FLIES,
		PATH_ORBIT,
		PATH_DRIP
	};

public:

	/// Get the shader name.
	virtual const std::string& getMaterialName() const = 0;

    /// Set the shader name.
	virtual void setMaterialName(const std::string& material) = 0;

	/// Get the particle count.
	virtual int getCount() const = 0;

	/// Set the particle count.
	virtual void setCount(int count) = 0;

	/**
	 * Get the duration in seconds.
	 */
	virtual float getDuration() const = 0;

	/**
	 * Set the duration in seconds, updates cyclemsec.
	 */
	virtual void setDuration(float duration) = 0;

	/**
	 * Returns ( duration + deadTime ) in msec.
	 */
	virtual int getCycleMsec() const = 0;

	/**
	 * Get the cycles value.
	 */
	virtual float getCycles() const = 0;

	/**
	 * Set the cycles value.
	 */
	virtual void setCycles(float cycles) = 0;

	/**
	 * Get the bunching value [0..1]
	 */
	virtual float getBunching() const = 0;

	/**
	 * Set the bunching value [0..1]
	 */
	virtual void setBunching(float value) = 0;

	/**
	 * Get the time offset in seconds
	 */
	virtual float getTimeOffset() const = 0;

	/**
	 * Set the time offset in seconds
	 */
	virtual void setTimeOffset(float value) = 0;

	/**
	 * Get the dead time in seconds
	 */
	virtual float getDeadTime() const = 0;

	/**
	 * Set the dead time in seconds, updates cyclemsec.
	 */
	virtual void setDeadTime(float value) = 0;

	/**
	 * Get the particle render colour.
	 */
	virtual const Vector4& getColour() const = 0;

	/**
	 * Set the particle render colour.
	 */
	virtual void setColour(const Vector4& colour) = 0;

	/**
	 * Get the particle render colour.
	 */
	virtual const Vector4& getFadeColour() const = 0;

	/**
	 * Set the particle render colour.
	 */
	virtual void setFadeColour(const Vector4& colour) = 0;

	/**
	 * Get the fade in fraction [0..1]
	 */
	virtual float getFadeInFraction() const = 0;

	/**
	 * Set the fade in fraction [0..1]
	 */
	virtual void setFadeInFraction(float fraction) = 0;

	/**
	 * Get the fade out fraction [0..1]
	 */
	virtual float getFadeOutFraction() const = 0;

	/**
	 * Set the fade out fraction [0..1]
	 */
	virtual void setFadeOutFraction(float fraction) = 0;

	/**
	 * Get the fade index fraction [0..1]
	 */
	virtual float getFadeIndexFraction() const = 0;

	/**
	 * Set the fade index fraction [0..1]
	 */
	virtual void setFadeIndexFraction(float fraction) = 0;

	/**
	 * Get the animation frames.
	 */
	virtual int getAnimationFrames() const = 0;

	/**
	 * Set the animation frames.
	 */
	virtual void setAnimationFrames(int animationFrames) = 0;

	/**
	 * Get the animation rate.
	 */
	virtual float getAnimationRate() const = 0;

	/**
	 * Set the animation frames.
	 */
	virtual void setAnimationRate(float animationRate) = 0;

	/**
	 * Get the initial angle.
	 */
	virtual float getInitialAngle() const = 0;

	/**
	 * Set the initial angle.
	 */
	virtual void setInitialAngle(float angle) = 0;

	/**
	 * Get the bounds expansion value.
	 */
	virtual float getBoundsExpansion() const = 0;

	/**
	 * Set the bounds expansion value.
	 */
	virtual void setBoundsExpansion(float value) = 0;

	/**
	 * Get the random distribution flag.
	 */
	virtual bool getRandomDistribution() const = 0;

	/**
	 * Set the random distribution flag.
	 */
	virtual void setRandomDistribution(bool value) = 0;

	/**
	 * Get the "use entity colour" flag.
	 */
	virtual bool getUseEntityColour() const = 0;

	/**
	 * Set the "use entity colour" flag.
	 */
	virtual void setUseEntityColour(bool value) = 0;

	/**
	 * Get the gravity factor.
	 */
	virtual float getGravity() const = 0;

	/**
	 * Set the gravity factor.
	 */
	virtual void setGravity(float value) = 0;

	/**
	 * Get the "apply gravity in world space" flag.
	 */
	virtual bool getWorldGravityFlag() const = 0;

	/**
	 * Get the "apply gravity in world space" flag.
	 */
	virtual void setWorldGravityFlag(bool value) = 0;

	/**
	 * Get the offset vector.
	 */
	virtual const Vector3& getOffset() const = 0;

	/**
	 * Set the offset vector.
	 */
	virtual void setOffset(const Vector3& value) = 0;

	/**
	 * Get the orientation type.
	 */
	virtual OrientationType getOrientationType() const = 0;

	/**
	 * Set the orientation type.
	 */
	virtual void setOrientationType(OrientationType value) = 0;

	/**
	 * Get the orientation parameter with the given index [0..3]
	 */
	virtual float getOrientationParm(int parmNum) const = 0;

	/*
	 * Set the orientation parameter with the given index [0..3].
	 */
	virtual void setOrientationParm(int parmNum, float value) = 0;

	/**
	 * Get the distribution type.
	 */
	virtual DistributionType getDistributionType() const = 0;

	/**
	 * Set the distribution type.
	 */
	virtual void setDistributionType(DistributionType value) = 0;

	/**
	 * Get the distribution parameter with the given index [0..3]
	 */
	virtual float getDistributionParm(int parmNum) const = 0;

	/*
	 * Set the distribution parameter with the given index [0..3].
	 */
	virtual void setDistributionParm(int parmNum, float value) = 0;

	/**
	 * Get the direction type.
	 */
	virtual DirectionType getDirectionType() const = 0;

	/**
	 * Set the direction type.
	 */
	virtual void setDirectionType(DirectionType value) = 0;

	/**
	 * Get the direction parameter with the given index [0..3]
	 */
	virtual float getDirectionParm(int parmNum) const = 0;

	/*
	 * Set the direction parameter with the given index [0..3].
	 */
	virtual void setDirectionParm(int parmNum, float value) = 0;

	/**
	 * Get the custom path type.
	 */
	virtual CustomPathType getCustomPathType() const = 0;

	/**
	 * Set the custom path type.
	 */
	virtual void setCustomPathType(CustomPathType value) = 0;

	/**
	 * Get the custom path parameter with the given index [0..7]
	 */
	virtual float getCustomPathParm(int parmNum) const = 0;

	/*
	 * Set the custom path parameter with the given index [0..7].
	 */
	virtual void setCustomPathParm(int parmNum, float value) = 0;

	/**
	 * Get the particle size
	 */
	virtual const IParticleParameter& getSize() const = 0;
	virtual IParticleParameter& getSize() = 0;

	/**
	 * Get the aspect ratio.
	 */
	virtual const IParticleParameter& getAspect() const = 0;
	virtual IParticleParameter& getAspect() = 0;

	/**
	 * Get the particle speed.
	 */
	virtual const IParticleParameter& getSpeed() const = 0;
	virtual IParticleParameter& getSpeed() = 0;

	/**
	 * Get the particle rotation speed.
	 */
	virtual const IParticleParameter& getRotationSpeed() const = 0;
	virtual IParticleParameter& getRotationSpeed() = 0;

	// Comparison operator - particle stages are considered equal 
	// if all properties are equal
    virtual bool isEqualTo(const Ptr& other) = 0;

	/**
	 * Copy operator, copies all properties from the other stage into this one.
	 */
	virtual void copyFrom(const Ptr& other) = 0;

	/**
	 * Returns the stage visibility. This flag is used in the Particle Editor context only,
	 * it is not saved to the .prt file therefore it has no effect within the Doom 3 engine.
	 */
	virtual bool isVisible() const = 0;

	/**
	 * Sets the stage visibility. This flag is used in the Particle Editor context only,
	 * it is not saved to the .prt file therefore it has no effect within the Doom 3 engine.
	 */
	virtual void setVisible(bool visible) = 0;
};

} // namespace
