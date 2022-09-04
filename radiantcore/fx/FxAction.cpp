#include "FxAction.h"

#include "parser/DefTokeniser.h"
#include "string/case_conv.h"

namespace fx
{

FxAction::FxAction() :
    _delayInSeconds(0.0),
    _ignoreMaster(false)
{}

float FxAction::getDelay()
{
    return _delayInSeconds;
}

bool FxAction::getIgnoreMaster()
{
    return _ignoreMaster;
}

void FxAction::parseFromTokens(parser::DefTokeniser& tokeniser)
{
    while (tokeniser.hasMoreTokens())
    {
        auto token = tokeniser.nextToken();
        string::to_lower(token);

        // Hit a closing brace and we're done with this action
        if (token == "}") return;
        
        if (token == "ignoremaster")
        {
            _ignoreMaster = true;
        }
        else if (token == "delay")
        {
            _delayInSeconds = string::convert<float>(tokeniser.nextToken());
        }
    }
}


}
