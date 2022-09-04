#pragma once

#include "ifx.h"

namespace fx
{

class FxAction :
    public IFxAction
{
private:
    float _delayInSeconds;
    bool _ignoreMaster;

public:
    using Ptr = std::shared_ptr<FxAction>;

    FxAction();

    float getDelay() override;
    bool getIgnoreMaster() override;

    // Parses the action from the given tokens.
    // The opening brace { will already have been been consumed by the calling code
    void parseFromTokens(parser::DefTokeniser& tokeniser);
};

}
