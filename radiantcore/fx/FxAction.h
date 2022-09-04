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

    float _delayInSeconds;

    float _shakeTime;
    float _shakeAmplitude;
    float _shakeDistance;
    bool _shakeFalloff;
    float _shakeImpulse;
    bool _ignoreMaster;

public:
    using Ptr = std::shared_ptr<FxAction>;

    FxAction(FxDeclaration& fx);

    Type getType() override;
    float getDelay() override;
    bool getIgnoreMaster() override;

    float getShakeTime() override;
    float getShakeAmplitude() override;
    float getShakeDistance() override;
    bool getShakeFalloff() override;
    float getShakeImpulse() override;

    // Parses the action from the given tokens.
    // The opening brace { will already have been been consumed by the calling code
    void parseFromTokens(parser::DefTokeniser& tokeniser);
};

}
