#include "FxAction.h"

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

}
