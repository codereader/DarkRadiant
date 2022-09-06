#pragma once

#include "ifx.h"

namespace fx
{

class FxDeclaration;

class FxAction :
    public IFxAction
{
private:
    FxDeclaration& _fx;

    Type _type;
    std::string _name;

    float _delayInSeconds;
    float _durationInSeconds;
    float _shakeTime;
    float _shakeAmplitude;
    float _shakeDistance;
    bool _shakeFalloff;
    float _shakeImpulse;
    bool _ignoreMaster;
    bool _noShadows;
    std::string _fireSiblingAction;
    std::pair<float, float> _randomDelay;
    float _rotate;
    bool _trackOrigin;
    bool _restart;
    float _fadeInTimeInSeconds;
    float _fadeOutTimeInSeconds;
    float _decalSize;
    Vector3 _offset;
    Vector3 _axis;
    Vector3 _angle;
    std::string _useLightAction;
    std::string _useModelAction;
    std::string _attachLightName;
    std::string _attachEntityName;
    std::string _launchProjectileDefName;
    std::string _lightMaterialName;
    Vector3 _lightRgbColour;
    float _lightRadius;

public:
    using Ptr = std::shared_ptr<FxAction>;

    FxAction(FxDeclaration& fx);

    Type getType() override;
    const std::string& getName() override;
    float getDelay() override;
    float getDuration() override;
    bool getIgnoreMaster() override;
    float getShakeTime() override;
    float getShakeAmplitude() override;
    float getShakeDistance() override;
    bool getShakeFalloff() override;
    float getShakeImpulse() override;
    bool getNoShadows() override;
    const std::string& getFireSiblingAction() override;
    std::pair<float, float> getRandomDelay() override;
    float getRotate() override;
    bool getTrackOrigin() override;
    bool getRestart() override;
    float getFadeInTimeInSeconds() override;
    float getFadeOutTimeInSeconds() override;
    float getDecalSize() override;
    const Vector3& getOffset() override;
    const Vector3& getAxis() override;
    const Vector3& getAngle() override;
    const std::string& getUseLight() override;
    const std::string& getUseModel() override;
    const std::string& getAttachLight() override;
    const std::string& getAttachEntity() override;
    const std::string& getLaunchProjectileDef() override;
    const std::string& getLightMaterialName() override;
    const Vector3& getLightRgbColour() override;
    float getLightRadius() override;

    // Parses the action from the given tokens.
    // The opening brace { will already have been been consumed by the calling code
    void parseFromTokens(parser::DefTokeniser& tokeniser);
};

}
