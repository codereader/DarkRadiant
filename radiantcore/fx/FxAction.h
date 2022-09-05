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

    // Parses the action from the given tokens.
    // The opening brace { will already have been been consumed by the calling code
    void parseFromTokens(parser::DefTokeniser& tokeniser);
};

}
