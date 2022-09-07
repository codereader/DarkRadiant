#pragma once

#include "ifx.h"
#include "DeclarationManagerInterface.h"

namespace script
{

class ScriptFxAction
{
private:
    fx::IFxAction::Ptr _action;

public:
    ScriptFxAction(const fx::IFxAction::Ptr& action) :
        _action(action)
    {}

    fx::IFxAction::Type getActionType()
    {
        return _action ? _action->getActionType() : fx::IFxAction::Type::Undefined;
    }

    std::string getName()
    {
        return _action ? _action->getName() : std::string();
    }

    float getDelayInSeconds()
    {
        return _action ? _action->getDelayInSeconds() : 0;
    }

    float getDurationInSeconds()
    {
        return _action ? _action->getDurationInSeconds() : 0;
    }

    bool getIgnoreMaster()
    {
        return _action ? _action->getIgnoreMaster() : false;
    }

    float getShakeTimeInSeconds()
    {
        return _action ? _action->getShakeTimeInSeconds() : 0;
    }

    float getShakeAmplitude()
    {
        return _action ? _action->getShakeAmplitude() : 0;
    }

    float getShakeDistance()
    {
        return _action ? _action->getShakeDistance() : 0;
    }

    bool getShakeFalloff()
    {
        return _action ? _action->getShakeFalloff() : false;
    }

    float getShakeImpulse()
    {
        return _action ? _action->getShakeImpulse() : 0;
    }

    bool getNoShadows()
    {
        return _action ? _action->getNoShadows() : false;
    }

    std::string getFireSiblingAction()
    {
        return _action ? _action->getFireSiblingAction() : std::string();
    }

    std::pair<float, float> getRandomDelay()
    {
        return _action ? _action->getRandomDelay() : std::pair<float, float>();
    }

    float getRotate()
    {
        return _action ? _action->getRotate() : 0;
    }

    bool getTrackOrigin()
    {
        return _action ? _action->getTrackOrigin() : false;
    }

    bool getRestart()
    {
        return _action ? _action->getRestart() : false;
    }

    float getFadeInTimeInSeconds()
    {
        return _action ? _action->getFadeInTimeInSeconds() : 0;
    }

    float getFadeOutTimeInSeconds()
    {
        return _action ? _action->getFadeOutTimeInSeconds() : 0;
    }

    float getDecalSize()
    {
        return _action ? _action->getDecalSize() : 0;
    }

    Vector3 getOffset()
    {
        return _action ? _action->getOffset() : Vector3(0,0,0);
    }

    Vector3 getAxis()
    {
        return _action ? _action->getAxis() : Vector3(0,0,0);
    }

    Vector3 getAngle()
    {
        return _action ? _action->getAngle() : Vector3(0,0,0);
    }

    std::string getUseLight()
    {
        return _action ? _action->getUseLight() : std::string();
    }

    std::string getUseModel()
    {
        return _action ? _action->getUseModel() : std::string();
    }

    std::string getAttachLight()
    {
        return _action ? _action->getAttachLight() : std::string();
    }

    std::string getAttachEntity()
    {
        return _action ? _action->getAttachEntity() : std::string();
    }

    std::string getLaunchProjectileDef()
    {
        return _action ? _action->getLaunchProjectileDef() : std::string();
    }

    std::string getLightMaterialName()
    {
        return _action ? _action->getLightMaterialName() : std::string();
    }

    Vector3 getLightRgbColour()
    {
        return _action ? _action->getLightRgbColour() : Vector3(0,0,0);
    }

    float getLightRadius()
    {
        return _action ? _action->getLightRadius() : 0;
    }

    std::string getModelName()
    {
        return _action ? _action->getModelName() : std::string();
    }

    std::string getDecalMaterialName()
    {
        return _action ? _action->getDecalMaterialName() : std::string();
    }

    bool getParticleTrackVelocity()
    {
        return _action ? _action->getParticleTrackVelocity() : false;
    }

    std::string getSoundShaderName()
    {
        return _action ? _action->getSoundShaderName() : std::string();
    }

    std::string getShockwaveDefName()
    {
        return _action ? _action->getShockwaveDefName() : std::string();
    }
};

class ScriptFxDeclaration :
    public ScriptDeclaration
{
private:
    fx::IFxDeclaration::Ptr _fx;

public:
    ScriptFxDeclaration(const fx::IFxDeclaration::Ptr& fx) :
        ScriptDeclaration(fx),
        _fx(fx)
    {}

    bool isNull()
    {
        return !_fx;
    }

    std::size_t getNumActions()
    {
        return _fx ? _fx->getNumActions() : 0;
    }

    ScriptFxAction getAction(std::size_t index)
    {
        return ScriptFxAction(_fx ? _fx->getAction(index) : fx::IFxAction::Ptr());
    }

    std::string getBindTo()
    {
        return _fx ? _fx->getBindTo() : std::string();
    }
};

/**
* Exposes the GlobalFxManager interface to scripts
*/
class FxManagerInterface :
    public IScriptInterface
{
public:
    // Mapped methods
    ScriptFxDeclaration findFx(const std::string& name);

    // IScriptInterface implementation
    void registerInterface(py::module& scope, py::dict& globals) override;
};

}
