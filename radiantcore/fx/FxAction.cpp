#include "FxAction.h"

#include "parser/DefTokeniser.h"
#include "string/case_conv.h"
#include "FxDeclaration.h"

namespace fx
{

FxAction::FxAction(FxDeclaration& fx) :
    _fx(fx),
    _type(Type::Undefined),
    _delayInSeconds(0.0),
    _shakeTime(0),
    _shakeAmplitude(0),
    _shakeDistance(0),
    _shakeFalloff(false),
    _shakeImpulse(0),
    _ignoreMaster(false),
    _noShadows(false)
{}

FxAction::Type FxAction::getType()
{
    return _type;
}

const std::string& FxAction::getName()
{
    return _name;
}

float FxAction::getDelay()
{
    return _delayInSeconds;
}

float FxAction::getShakeTime()
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
#if 0
        if (!token.Icmp("random")) {
            FXAction.random1 = src.ParseFloat();
            src.ExpectTokenString(",");
            FXAction.random2 = src.ParseFloat();
            FXAction.delay = 0.0f;		// check random
            continue;
        }

        if (!token.Icmp("delay")) {
            FXAction.delay = src.ParseFloat();
            continue;
        }

        if (!token.Icmp("rotate")) {
            FXAction.rotate = src.ParseFloat();
            continue;
        }

        if (!token.Icmp("duration")) {
            FXAction.duration = src.ParseFloat();
            continue;
        }

        if (!token.Icmp("trackorigin")) {
            FXAction.trackOrigin = src.ParseBool();
            continue;
        }

        if (!token.Icmp("restart")) {
            FXAction.restart = src.ParseFloat();
            continue;
        }

        if (!token.Icmp("fadeIn")) {
            FXAction.fadeInTime = src.ParseFloat();
            continue;
        }

        if (!token.Icmp("fadeOut")) {
            FXAction.fadeOutTime = src.ParseFloat();
            continue;
        }

        if (!token.Icmp("size")) {
            FXAction.size = src.ParseFloat();
            continue;
        }

        if (!token.Icmp("offset")) {
            FXAction.offset.x = src.ParseFloat();
            src.ExpectTokenString(",");
            FXAction.offset.y = src.ParseFloat();
            src.ExpectTokenString(",");
            FXAction.offset.z = src.ParseFloat();
            continue;
        }

        if (!token.Icmp("axis")) {
            idVec3 v;
            v.x = src.ParseFloat();
            src.ExpectTokenString(",");
            v.y = src.ParseFloat();
            src.ExpectTokenString(",");
            v.z = src.ParseFloat();
            v.Normalize();
            FXAction.axis = v.ToMat3();
            FXAction.explicitAxis = true;
            continue;
        }

        if (!token.Icmp("angle")) {
            idAngles a;
            a[0] = src.ParseFloat();
            src.ExpectTokenString(",");
            a[1] = src.ParseFloat();
            src.ExpectTokenString(",");
            a[2] = src.ParseFloat();
            FXAction.axis = a.ToMat3();
            FXAction.explicitAxis = true;
            continue;
        }

        if (!token.Icmp("uselight")) {
            src.ReadToken(&token);
            FXAction.data = token;
            for (int i = 0; i < events.Num(); i++) {
                if (events[i].name.Icmp(FXAction.data) == 0) {
                    FXAction.sibling = i;
                    FXAction.lightColor = events[i].lightColor;
                    FXAction.lightRadius = events[i].lightRadius;
                }
            }
            FXAction.type = FX_LIGHT;

            // precache the light material
            declManager->FindMaterial(FXAction.data);
            continue;
        }

        if (!token.Icmp("attachlight")) {
            src.ReadToken(&token);
            FXAction.data = token;
            FXAction.type = FX_ATTACHLIGHT;

            // precache it
            declManager->FindMaterial(FXAction.data);
            continue;
        }

        if (!token.Icmp("attachentity")) {
            src.ReadToken(&token);
            FXAction.data = token;
            FXAction.type = FX_ATTACHENTITY;

            // precache the model
            renderModelManager->FindModel(FXAction.data);
            continue;
        }

        if (!token.Icmp("launch")) {
            src.ReadToken(&token);
            FXAction.data = token;
            FXAction.type = FX_LAUNCH;

            // precache the entity def
            declManager->FindType(DECL_ENTITYDEF, FXAction.data);
            continue;
        }

        if (!token.Icmp("useModel")) {
            src.ReadToken(&token);
            FXAction.data = token;
            for (int i = 0; i < events.Num(); i++) {
                if (events[i].name.Icmp(FXAction.data) == 0) {
                    FXAction.sibling = i;
                }
            }
            FXAction.type = FX_MODEL;

            // precache the model
            renderModelManager->FindModel(FXAction.data);
            continue;
        }

        if (!token.Icmp("light")) {
            src.ReadToken(&token);
            FXAction.data = token;
            src.ExpectTokenString(",");
            FXAction.lightColor[0] = src.ParseFloat();
            src.ExpectTokenString(",");
            FXAction.lightColor[1] = src.ParseFloat();
            src.ExpectTokenString(",");
            FXAction.lightColor[2] = src.ParseFloat();
            src.ExpectTokenString(",");
            FXAction.lightRadius = src.ParseFloat();
            FXAction.type = FX_LIGHT;

            // precache the light material
            declManager->FindMaterial(FXAction.data);
            continue;
        }

        if (!token.Icmp("model")) {
            src.ReadToken(&token);
            FXAction.data = token;
            FXAction.type = FX_MODEL;

            // precache it
            renderModelManager->FindModel(FXAction.data);
            continue;
        }

        if (!token.Icmp("particle")) {	// FIXME: now the same as model
            src.ReadToken(&token);
            FXAction.data = token;
            FXAction.type = FX_PARTICLE;

            // precache it
            renderModelManager->FindModel(FXAction.data);
            continue;
        }

        if (!token.Icmp("decal")) {
            src.ReadToken(&token);
            FXAction.data = token;
            FXAction.type = FX_DECAL;

            // precache it
            declManager->FindMaterial(FXAction.data);
            continue;
        }

        if (!token.Icmp("particleTrackVelocity")) {
            FXAction.particleTrackVelocity = true;
            continue;
        }

        if (!token.Icmp("sound")) {
            src.ReadToken(&token);
            FXAction.data = token;
            FXAction.type = FX_SOUND;

            // precache it
            declManager->FindSound(FXAction.data);
            continue;
        }

        if (!token.Icmp("ignoreMaster")) {
            FXAction.shakeIgnoreMaster = true;
            continue;
        }

        if (!token.Icmp("shockwave")) {
            src.ReadToken(&token);
            FXAction.data = token;
            FXAction.type = FX_SHOCKWAVE;

            // precache the entity def
            declManager->FindType(DECL_ENTITYDEF, FXAction.data);
            continue;
        }
#endif
        else
        {
            rWarning() << "Unrecognised token '" << token << "' in FX " << _fx.getDeclName() << std::endl;
        }
    }
}


}
