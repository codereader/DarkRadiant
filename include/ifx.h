#pragma once

#include "ideclmanager.h"

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

    virtual Type getType() = 0;

    // Returns the name of this action (which might be an empty string)
    virtual const std::string& getName() = 0;

    // Returns the action delay in seconds
    virtual float getDelay() = 0;

    // True: Don't shake the entity this effect is attached to
    virtual bool getIgnoreMaster() = 0;

    // Shake parameters

    virtual float getShakeTime() = 0;
    virtual float getShakeAmplitude() = 0;
    virtual float getShakeDistance() = 0;
    virtual bool getShakeFalloff() = 0;
    virtual float getShakeImpulse() = 0;

    // True: The light in this effect doesn't cast shadows
    virtual bool getNoShadows() = 0;

    // Causes the sibling action to happen when this action does.
    virtual const std::string& getFireSiblingAction() = 0;
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

}
