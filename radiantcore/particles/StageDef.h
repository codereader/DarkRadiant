#pragma once

#include "math/Vector4.h"

#include "iparticles.h"

#include <iostream>
#include "parser/DefTokeniser.h"

#include "ParticleParameter.h"

namespace particles
{

class ParticleDef;

/// Implementation of IStageDef for idTech 4 particles
class StageDef : public IStageDef
{
    friend std::ostream& operator<< (std::ostream&, const StageDef&);

    // Number of particles
    int _count;                     // total number of particles, although some may be invisible at a given time

    // Material to render onto each quad
    std::string _material;

    float _duration;                // Duration in seconds
    float _cycles;                  // allows things to oneShot ( 1 cycle ) or run for a set number of cycles
                                    // on a per stage basis
    float _bunching;                // 0.0 = all come out at first instant, 1.0 = evenly spaced over cycle time

    float _timeOffset;              // time offset from system start for the first particle to spawn
    float _deadTime;                // time after particleLife before respawning

    int _cycleMsec;                 // calculated as ( _duration + _deadTime ) in msec, read-only for public

    Vector4 _colour;                // Render colour
    Vector4 _fadeColour;            // either 0 0 0 0 for additive, or 1 1 1 0 for blended materials

    float _fadeInFraction;          // in 0.0 to 1.0 range
    float _fadeOutFraction;         // in 0.0 to 1.0 range
    float _fadeIndexFraction;       // in 0.0 to 1.0 range, causes later index smokes to be more faded

    int _animationFrames;           // if > 1, subdivide the texture S axis into frames and crossfade
    float _animationRate;           // frames per second

    float _initialAngle;            // in degrees, random angle is used if zero ( default )

    ParticleParameterPtr _rotationSpeed; // half the particles will have negative rotation speeds,
                                      // this is measured in degrees/sec

    float _boundsExpansion;         // user tweak to fix poorly calculated bounds

    bool _randomDistribution;       // randomly orient the quad on emission ( defaults to true )
    bool _entityColor;              // force color from render entity ( fadeColor is still valid )

    float _gravity;                 // can be negative to float up
    bool _applyWorldGravity;        // apply gravity in world space

    Vector3 _offset;                // offset from origin to spawn all particles, also applies to customPath

    OrientationType _orientationType;   // view, aimed, or axis fixed
    float _orientationParms[4];         // Orientation parameters

    // Standard path parms

    DistributionType _distributionType; // Distribution type
    float _distributionParms[4];        // Distribution parameters

    DirectionType _directionType;   // Direction type
    float _directionParms[4];       // Direction parameters

    ParticleParameterPtr _speed;        // Speed

    // Custom path will completely replace the standard path calculations

    CustomPathType _customPathType; // use custom C code routines for determining the origin
    float _customPathParms[8];      // custom path parameters

    ParticleParameterPtr _size;     // Size

    ParticleParameterPtr _aspect;       // greater than 1 makes the T axis longer

    // whether this stage is visible, used within the particle editor only
    bool _visible;

    sigc::signal<void> _changedSignal;

private:

    /// Resets/clears all values to default. This is called by parseFromTokens().
    void reset();

    void recalculateCycleMsec()
    {
        _cycleMsec = static_cast<int>((_duration + _deadTime) * 1000);
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

public:
    using Ptr = std::shared_ptr<StageDef>;

    /// Create an empty particle stage with default values
    StageDef();

    /// Create a particle stage from the given token stream
    static Ptr Parse(parser::DefTokeniser& tok);

    /// Signal emitted when some property of the particle stage has changed
    sigc::signal<void> signal_changed() const
    {
        return _changedSignal;
    }

    // IStageDef implementation
    const std::string& getMaterialName() const { return _material; }
    void setMaterialName(const std::string& material);
    int getCount() const { return _count; }
    void setCount(int count)
    {
        _count = count;
        _changedSignal.emit();
    }

    /**
     * Get the duration in seconds.
     */
    float getDuration() const { return _duration; }

    /**
     * Set the duration in seconds, updates cyclemsec.
     */
    void setDuration(float duration)
    {
        _duration = duration;
        recalculateCycleMsec();
        _changedSignal.emit();
    }

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
    void setCycles(float cycles)
    {
        _cycles = clampZeroOrPositive(cycles);
        _changedSignal.emit();
    }

    /**
     * Get the bunching value [0..1]
     */
    float getBunching() const { return _bunching; }

    /**
     * Set the bunching value [0..1]
     */
    void setBunching(float value)
    {
        _bunching = clampOneZero(value);
        _changedSignal.emit();
    }

    /**
     * Get the time offset in seconds
     */
    float getTimeOffset() const { return _timeOffset; }

    /**
     * Set the time offset in seconds
     */
    void setTimeOffset(float value) { _timeOffset = value; _changedSignal.emit(); }

    /**
     * Get the dead time in seconds
     */
    float getDeadTime() const { return _deadTime; }

    /**
     * Set the dead time in seconds, updates cyclemsec.
     */
    void setDeadTime(float value)
    {
        _deadTime = value;
        recalculateCycleMsec();
        _changedSignal.emit();
    }

    /**
     * Get the particle render colour.
     */
    const Vector4& getColour() const { return _colour; }

    /**
     * Set the particle render colour.
     */
    void setColour(const Vector4& colour) { _colour = colour; _changedSignal.emit(); }

    /**
     * Get the particle render colour.
     */
    const Vector4& getFadeColour() const { return _fadeColour; }

    /**
     * Set the particle render colour.
     */
    void setFadeColour(const Vector4& colour) { _fadeColour = colour; _changedSignal.emit(); }

    /**
     * Get the fade in fraction [0..1]
     */
    float getFadeInFraction() const { return _fadeInFraction; }

    /**
     * Set the fade in fraction [0..1]
     */
    void setFadeInFraction(float fraction)
    {
        _fadeInFraction = clampOneZero(fraction);
        _changedSignal.emit();
    }

    /**
     * Get the fade out fraction [0..1]
     */
    float getFadeOutFraction() const { return _fadeOutFraction; }

    /**
     * Set the fade out fraction [0..1]
     */
    void setFadeOutFraction(float fraction)
    {
        _fadeOutFraction = clampOneZero(fraction);
        _changedSignal.emit();
    }

    /**
     * Get the fade index fraction [0..1]
     */
    float getFadeIndexFraction() const { return _fadeIndexFraction; }

    /**
     * Set the fade index fraction [0..1]
     */
    void setFadeIndexFraction(float fraction)
    {
        _fadeIndexFraction = clampOneZero(fraction);
        _changedSignal.emit();
    }

    /**
     * Get the animation frames.
     */
    int getAnimationFrames() const { return _animationFrames; }

    /**
     * Set the animation frames.
     */
    void setAnimationFrames(int animationFrames)
    {
        _animationFrames = animationFrames;
        _changedSignal.emit();
    }

    /**
     * Get the animation rate.
     */
    float getAnimationRate() const { return _animationRate; }

    /**
     * Set the animation frames.
     */
    void setAnimationRate(float animationRate) { _animationRate = animationRate; _changedSignal.emit(); }

    /**
     * Get the initial angle.
     */
    float getInitialAngle() const { return _initialAngle; }

    /**
     * Set the initial angle.
     */
    void setInitialAngle(float angle) { _initialAngle = angle; _changedSignal.emit(); }

    /**
     * Get the bounds expansion value.
     */
    float getBoundsExpansion() const { return _boundsExpansion; }

    /**
     * Set the bounds expansion value.
     */
    void setBoundsExpansion(float value) { _boundsExpansion = value; _changedSignal.emit(); }

    /**
     * Get the random distribution flag.
     */
    bool getRandomDistribution() const { return _randomDistribution; }

    /**
     * Set the random distribution flag.
     */
    void setRandomDistribution(bool value) { _randomDistribution = value; _changedSignal.emit(); }

    /**
     * Get the "use entity colour" flag.
     */
    bool getUseEntityColour() const { return _entityColor; }

    /**
     * Set the "use entity colour" flag.
     */
    void setUseEntityColour(bool value) { _entityColor = value; _changedSignal.emit(); }

    /**
     * Get the gravity factor.
     */
    float getGravity() const { return _gravity; }

    /**
     * Set the gravity factor.
     */
    void setGravity(float value) { _gravity = value; _changedSignal.emit(); }

    /**
     * Get the "apply gravity in world space" flag.
     */
    bool getWorldGravityFlag() const { return _applyWorldGravity; }

    /**
     * Get the "apply gravity in world space" flag.
     */
    void setWorldGravityFlag(bool value)
    {
        _applyWorldGravity = value;
        _changedSignal.emit();
    }

    /**
     * Get the offset vector.
     */
    const Vector3& getOffset() const { return _offset; }

    /**
     * Set the offset vector.
     */
    void setOffset(const Vector3& value)
    {
        _offset = value;
        _changedSignal.emit();
    }

    /**
     * Get the orientation type.
     */
    OrientationType getOrientationType() const { return _orientationType; }

    /**
     * Set the orientation type.
     */
    void setOrientationType(OrientationType value)
    {
        _orientationType = value;
        _changedSignal.emit();
    }

    /**
     * Get the orientation parameter with the given index [0..3]
     */
    float getOrientationParm(int parmNum) const
    {
        assert(parmNum >= 0 && parmNum < 4);
        return _orientationParms[parmNum];
    }

    /*
     * Set the orientation parameter with the given index [0..3].
     */
    void setOrientationParm(int parmNum, float value)
    {
        assert(parmNum >= 0 && parmNum < 4);
        _orientationParms[parmNum] = value;

        _changedSignal.emit();
    }

    /**
     * Get the distribution type.
     */
    DistributionType getDistributionType() const { return _distributionType; }

    /**
     * Set the distribution type.
     */
    void setDistributionType(DistributionType value) { _distributionType = value; _changedSignal.emit(); }

    /**
     * Get the distribution parameter with the given index [0..3]
     */
    float getDistributionParm(int parmNum) const
    {
        assert(parmNum >= 0 && parmNum < 4);
        return _distributionParms[parmNum];
    }

    /*
     * Set the distribution parameter with the given index [0..3].
     */
    void setDistributionParm(int parmNum, float value)
    {
        assert(parmNum >= 0 && parmNum < 4);
        _distributionParms[parmNum] = value;

        _changedSignal.emit();
    }

    /**
     * Get the direction type.
     */
    DirectionType getDirectionType() const { return _directionType; }

    /**
     * Set the direction type.
     */
    void setDirectionType(DirectionType value) { _directionType = value; _changedSignal.emit(); }

    /**
     * Get the direction parameter with the given index [0..3]
     */
    float getDirectionParm(int parmNum) const
    {
        assert(parmNum >= 0 && parmNum < 4);
        return _directionParms[parmNum];
    }

    /*
     * Set the direction parameter with the given index [0..3].
     */
    void setDirectionParm(int parmNum, float value)
    {
        assert(parmNum >= 0 && parmNum < 4);
        _directionParms[parmNum] = value;

        _changedSignal.emit();
    }

    /**
     * Get the custom path type.
     */
    CustomPathType getCustomPathType() const { return _customPathType; }

    /**
     * Set the custom path type.
     */
    void setCustomPathType(CustomPathType value) { _customPathType = value; _changedSignal.emit(); }

    /**
     * Get the custom path parameter with the given index [0..7]
     */
    float getCustomPathParm(int parmNum) const
    {
        assert(parmNum >= 0 && parmNum < 8);
        return _customPathParms[parmNum];
    }

    /*
     * Set the custom path parameter with the given index [0..7].
     */
    void setCustomPathParm(int parmNum, float value)
    {
        assert(parmNum >= 0 && parmNum < 8);
        _customPathParms[parmNum] = value;

        _changedSignal.emit();
    }

    /**
     * Get the particle size
     */
    const ParticleParameter& getSize() const { return *_size; }
    ParticleParameter& getSize() { return *_size; }

    /**
     * Get the aspect ratio.
     */
    const ParticleParameter& getAspect() const { return *_aspect; }
    ParticleParameter& getAspect() { return *_aspect; }

    /**
     * Get the particle speed.
     */
    const ParticleParameter& getSpeed() const { return *_speed; }
    ParticleParameter& getSpeed() { return *_speed; }

    /**
     * Get the particle rotation speed.
     */
    const ParticleParameter& getRotationSpeed() const { return *_rotationSpeed; }
    ParticleParameter& getRotationSpeed() { return *_rotationSpeed; }

    /// Equality comparison with other IStageDef objects
    bool isEqualTo(const IStageDef::Ptr& other) override;

    void copyFrom(const IStageDef::Ptr& other);

    bool isVisible() const
    {
        return _visible;
    }

    void setVisible(bool visible)
    {
        _visible = visible;
    }

    // Called by the ParticleParameter classes on modification
    void onParameterChanged()
    {
        _changedSignal.emit();
    }

    // Parser method, reads in all stage parameters from the given token stream
    // The initial opening brace { has already been parsed.
    // The routine will continue parsing until the matching closing } is encountered.
    void parseFromTokens(parser::DefTokeniser& tok);

};

/**
 * Write stage to text stream, including opening & closing braces and one level of indentation.
 */
std::ostream& operator<< (std::ostream& os, const StageDef& stage);

}
