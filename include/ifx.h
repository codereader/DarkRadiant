#pragma once

#include "imodule.h"
#include "ideclmanager.h"
#include "math/Vector3.h"

namespace fx
{

class IFxAction
{
public:
    using Ptr = std::shared_ptr<IFxAction>;

    enum class Type
    {
        Undefined = -1,
        Light = 0,
        Particle,
        Decal,
        Model,
        Sound,
        Shake,
        AttachLight,
        AttachEntity,
        Launch,
        Shockwave
    };

    virtual ~IFxAction() {}

    virtual Type getActionType() = 0;

    // Returns the name of this action (which might be an empty string)
    virtual const std::string& getName() = 0;

    // Returns the action delay in seconds
    virtual float getDelayInSeconds() = 0;

    // Action duration in seconds, before it is killed or restarted
    virtual float getDurationInSeconds() = 0;

    // True: Don't shake the entity this effect is attached to
    virtual bool getIgnoreMaster() = 0;

    // Shake parameters

    virtual float getShakeTimeInSeconds() = 0;
    virtual float getShakeAmplitude() = 0;
    virtual float getShakeDistance() = 0;
    virtual bool getShakeFalloff() = 0;
    virtual float getShakeImpulse() = 0;

    // True: The light in this effect doesn't cast shadows
    virtual bool getNoShadows() = 0;

    // Causes the sibling action to happen when this action does.
    virtual const std::string& getFireSiblingAction() = 0;

    // Let the delay be random between min and max (in seconds)
    // If both are 0.0 no random delay is active and the regular delay is used instead
    virtual std::pair<float, float> getRandomDelay() = 0;

    // According to the docs this is not used
    virtual float getRotate() = 0;

    // Move around with the entity (vs stationary after spawning)
    virtual bool getTrackOrigin() = 0;

    // True: the action starts again after the 'duration' has run out
    virtual bool getRestart() = 0;

    // Fade in the RGB of the light or model over <time> seconds
    virtual float getFadeInTimeInSeconds() = 0;

    // Fade out the light/model. Ignored if fadeIn is set, you can use 2 separate
    // actions (tied together with uselight) if you want a light to fade in and out.
    virtual float getFadeOutTimeInSeconds() = 0;

    // Size of the decal (corresponds to "size" keyword)
    virtual float getDecalSize() = 0;

    // Offset from the origin of the entity (or bind point) this action is located at
    virtual const Vector3& getOffset() = 0;

    // Axis of the model, mutually exclusive with angle
    virtual const Vector3& getAxis() = 0;

    // Alternate way of setting the axis of the model
    virtual const Vector3& getAngle() = 0;

    // Returns the name of the action containing the light which should be used
    virtual const std::string& getUseLight() = 0;

    // Modify the model in a named sibling action. Can be used to fade out a particle in a sibling.
    virtual const std::string& getUseModel() = 0;

    // Attach to external light (a light not defined in the effect) for fading.
    virtual const std::string& getAttachLight() = 0;

    // Attach to an external entity
    virtual const std::string& getAttachEntity() = 0;

    // Launches a projectile of the given entityDef
    virtual const std::string& getLaunchProjectileDef() = 0;

    // If not empty, this action spawns a light with this material
    virtual const std::string& getLightMaterialName() = 0;

    // For Type::Light actions, this defines the RGB colour components
    virtual const Vector3& getLightRgbColour() = 0;

    // For Type::Light actions, this defines the radius of the spawned light
    virtual float getLightRadius() = 0;

    // Return the name of the model or particle
    virtual const std::string& getModelName() = 0;

    // For Type::Decal actions, this defines the decal material name
    virtual const std::string& getDecalMaterialName() = 0;

    // Unused according to docs
    virtual bool getParticleTrackVelocity() = 0;

    // For Type::Sound actions: start a sound (on any channel)
    virtual const std::string& getSoundShaderName() = 0;

    // For Type::Shockwave actions: the name of the shockwave entityDef
    virtual const std::string& getShockwaveDefName() = 0;
};

class IFxDeclaration :
    public decl::IDeclaration
{
public:
    using Ptr = std::shared_ptr<IFxDeclaration>;

    // Returns the number of actions in this FX declaration
    virtual std::size_t getNumActions() = 0;

    // Returns the n-th action (based on the given 0-based index)
    virtual IFxAction::Ptr getAction(std::size_t index) = 0;

    // Returns the name of the joint this FX should bind to
    // Evaluated to an empty string if "bindTo" is not set
    virtual std::string getBindTo() = 0;
};

/**
 * Convenience interface to deal with FX declarations
 */
class IFxManager :
    public RegisterableModule
{
public:
    /**
     * Lookup an FX declaration by name.
     * If the decl is not found, an empty reference is returned.
     */
    virtual IFxDeclaration::Ptr findFx(const std::string& name) = 0;
};

}

constexpr const char* const MODULE_FXMANAGER("FxManager");

inline fx::IFxManager& GlobalFxManager()
{
    static module::InstanceReference<fx::IFxManager> _reference(MODULE_FXMANAGER);
    return _reference;
}
