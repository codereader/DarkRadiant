#include "FxAction.h"

#include "string/convert.h"
#include "string/case_conv.h"
#include "FxDeclaration.h"

namespace fx
{

FxAction::FxAction(FxDeclaration& fx) :
    _fx(fx),
    _type(Type::Undefined),
    _delayInSeconds(0),
    _durationInSeconds(0),
    _shakeTime(0),
    _shakeAmplitude(0),
    _shakeDistance(0),
    _shakeFalloff(false),
    _shakeImpulse(0),
    _ignoreMaster(false),
    _noShadows(false),
    _randomDelay(0.0f, 0.0f),
    _rotate(0),
    _trackOrigin(false),
    _restart(false),
    _fadeInTimeInSeconds(0),
    _fadeOutTimeInSeconds(0),
    _decalSize(0),
    _offset(0,0,0),
    _axis(0,0,0),
    _angle(0,0,0),
    _lightRgbColour(0, 0, 0),
    _lightRadius(0),
    _particleTrackVelocity(false)
{}

FxAction::Type FxAction::getActionType()
{
    return _type;
}

const std::string& FxAction::getName()
{
    return _name;
}

float FxAction::getDelayInSeconds()
{
    return _delayInSeconds;
}

float FxAction::getShakeTimeInSeconds()
{
    return _shakeTime;
}

float FxAction::getShakeAmplitude()
{
    return _shakeAmplitude;
}

float FxAction::getShakeDistance()
{
    return _shakeDistance;
}

bool FxAction::getShakeFalloff()
{
    return _shakeFalloff;
}

float FxAction::getShakeImpulse()
{
    return _shakeImpulse;
}

bool FxAction::getIgnoreMaster()
{
    return _ignoreMaster;
}

bool FxAction::getNoShadows()
{
    return _noShadows;
}

const std::string& FxAction::getFireSiblingAction()
{
    return _fireSiblingAction;
}

std::pair<float, float> FxAction::getRandomDelay()
{
    return _randomDelay;
}

float FxAction::getRotate()
{
    return _rotate;
}

float FxAction::getDurationInSeconds()
{
    return _durationInSeconds;
}

bool FxAction::getTrackOrigin()
{
    return _trackOrigin;
}

bool FxAction::getRestart()
{
    return _restart;
}

float FxAction::getFadeInTimeInSeconds()
{
    return _fadeInTimeInSeconds;
}

float FxAction::getFadeOutTimeInSeconds()
{
    return _fadeOutTimeInSeconds;
}

float FxAction::getDecalSize()
{
    return _decalSize;
}

const Vector3& FxAction::getOffset()
{
    return _offset;
}

const Vector3& FxAction::getAxis()
{
    return _axis;
}

const Vector3& FxAction::getAngle()
{
    return _angle;
}

const std::string& FxAction::getUseLight()
{
    return _useLightAction;
}

const std::string& FxAction::getUseModel()
{
    return _useModelAction;
}

const std::string& FxAction::getAttachLight()
{
    return _attachLightName;
}

const std::string& FxAction::getAttachEntity()
{
    return _attachEntityName;
}

const std::string& FxAction::getLaunchProjectileDef()
{
    return _launchProjectileDefName;
}

const std::string& FxAction::getLightMaterialName()
{
    return _lightMaterialName;
}

const Vector3& FxAction::getLightRgbColour()
{
    return _lightRgbColour;
}

float FxAction::getLightRadius()
{
    return _lightRadius;
}

const std::string& FxAction::getModelName()
{
    return _modelName;
}

const std::string& FxAction::getDecalMaterialName()
{
    return _decalMaterialName;
}

bool FxAction::getParticleTrackVelocity()
{
    return _particleTrackVelocity;
}

const std::string& FxAction::getSoundShaderName()
{
    return _soundShaderName;
}

const std::string& FxAction::getShockwaveDefName()
{
    return _shockwaveDefName;
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
        else if (token == "shake")
        {
            // shake <time>,<amplitude>,<distance>,<falloff>,<impulse>
            _type = Type::Shake;
            _shakeTime = string::convert<float>(tokeniser.nextToken());
            tokeniser.assertNextToken(",");
            _shakeAmplitude = string::convert<float>(tokeniser.nextToken());
            tokeniser.assertNextToken(",");
            _shakeDistance = string::convert<float>(tokeniser.nextToken());
            tokeniser.assertNextToken(",");
            _shakeFalloff = string::convert<int>(tokeniser.nextToken()) != 0;
            tokeniser.assertNextToken(",");
            _shakeImpulse = string::convert<float>(tokeniser.nextToken());
        }
        else if (token == "noshadows")
        {
            _noShadows = true;
        }
        else if (token == "name")
        {
            _name = tokeniser.nextToken();
        }
        else if (token == "fire")
        {
            _fireSiblingAction = tokeniser.nextToken();
        }
        else if (token == "random")
        {
            _randomDelay.first = string::convert<float>(tokeniser.nextToken());
            tokeniser.assertNextToken(",");
            _randomDelay.second = string::convert<float>(tokeniser.nextToken());
        }
        else if (token == "rotate")
        {
            _rotate = string::convert<float>(tokeniser.nextToken());
        }
        else if (token == "duration")
        {
            _durationInSeconds = string::convert<float>(tokeniser.nextToken());
        }
        else if (token == "trackorigin")
        {
            _trackOrigin = string::convert<int>(tokeniser.nextToken()) != 0;
        }
        else if (token == "restart")
        {
            _restart = string::convert<int>(tokeniser.nextToken()) != 0;
        }
        else if (token == "fadein")
        {
            _fadeInTimeInSeconds = string::convert<float>(tokeniser.nextToken());
        }
        else if (token == "fadeout")
        {
            _fadeOutTimeInSeconds = string::convert<float>(tokeniser.nextToken());
        }
        else if (token == "size")
        {
            _decalSize = string::convert<float>(tokeniser.nextToken());
        }
        else if (token == "offset")
        {
            _offset.x() = string::convert<float>(tokeniser.nextToken());
            tokeniser.assertNextToken(",");
            _offset.y() = string::convert<float>(tokeniser.nextToken());
            tokeniser.assertNextToken(",");
            _offset.z() = string::convert<float>(tokeniser.nextToken());
        }
        else if (token == "axis")
        {
            _axis.x() = string::convert<float>(tokeniser.nextToken());
            tokeniser.assertNextToken(",");
            _axis.y() = string::convert<float>(tokeniser.nextToken());
            tokeniser.assertNextToken(",");
            _axis.z() = string::convert<float>(tokeniser.nextToken());
        }
        else if (token == "angle")
        {
            _angle.x() = string::convert<float>(tokeniser.nextToken());
            tokeniser.assertNextToken(",");
            _angle.y() = string::convert<float>(tokeniser.nextToken());
            tokeniser.assertNextToken(",");
            _angle.z() = string::convert<float>(tokeniser.nextToken());
        }
        else if (token == "uselight")
        {
            _useLightAction = tokeniser.nextToken();
            _type = Type::Light;
        }
        else if (token == "usemodel")
        {
            _useModelAction = tokeniser.nextToken();
            _type = Type::Model;
        }
        else if (token == "attachlight")
        {
            _attachLightName = tokeniser.nextToken();
            _type = Type::AttachLight;
        }
        else if (token == "attachentity")
        {
            _attachEntityName = tokeniser.nextToken();
            _type = Type::AttachEntity;
        }
        else if (token == "launch")
        {
            _launchProjectileDefName = tokeniser.nextToken();
            _type = Type::Launch;
        }
        else if (token == "light")
        {
            _type = Type::Light;
            _lightMaterialName = tokeniser.nextToken();
            tokeniser.assertNextToken(",");
            _lightRgbColour.x() = string::convert<float>(tokeniser.nextToken());
            tokeniser.assertNextToken(",");
            _lightRgbColour.y() = string::convert<float>(tokeniser.nextToken());
            tokeniser.assertNextToken(",");
            _lightRgbColour.z() = string::convert<float>(tokeniser.nextToken());
            tokeniser.assertNextToken(",");
            _lightRadius = string::convert<float>(tokeniser.nextToken());
        }
        else if (token == "model")
        {
            _type = Type::Model;
            _modelName = tokeniser.nextToken();
        }
        else if (token == "particle")
        {
            _type = Type::Particle;
            _modelName = tokeniser.nextToken();
        }
        else if (token == "decal")
        {
            _type = Type::Decal;
            _decalMaterialName = tokeniser.nextToken();
        }
        else if (token == "particletrackvelocity")
        {
            _particleTrackVelocity = true;
        }
        else if (token == "sound")
        {
            _type = Type::Sound;
            _soundShaderName = tokeniser.nextToken();
        }
        else if (token == "shockwave")
        {
            _type = Type::Shockwave;
            _shockwaveDefName = tokeniser.nextToken();
        }
        else
        {
            rWarning() << "Unrecognised token '" << token << "' in FX " << _fx.getDeclName() << std::endl;
        }
    }
}


}
