#include "ifx.h"

#include "RadiantTest.h"

namespace test
{

using FxTest = RadiantTest;

inline fx::IFxDeclaration::Ptr getFxByName(const std::string& name)
{
    return GlobalFxManager().findFx(name);
}

TEST_F(FxTest, GetFxByName)
{
    EXPECT_TRUE(GlobalFxManager().findFx("fx/tdm_flame"));
    EXPECT_TRUE(GlobalFxManager().findFx("fx/sparks"));
}

TEST_F(FxTest, GetNumActions)
{
    EXPECT_EQ(getFxByName("fx/tdm_flame")->getNumActions(), 1);
    EXPECT_EQ(getFxByName("fx/sparks")->getNumActions(), 3);
}

TEST_F(FxTest, GetAction)
{
    EXPECT_TRUE(getFxByName("fx/tdm_flame")->getAction(0));
    EXPECT_TRUE(getFxByName("fx/sparks")->getAction(1));

    // Out of range requests will throw
    EXPECT_THROW(getFxByName("fx/tdm_flame")->getAction(1), std::out_of_range);
    EXPECT_THROW(getFxByName("fx/tdm_flame")->getAction(10), std::out_of_range);
}

TEST_F(FxTest, ParseBindTo)
{
    EXPECT_EQ(getFxByName("fx/tdm_flame")->getBindTo(), "Head");
}

TEST_F(FxTest, ParseActionDelay)
{
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(0)->getDelayInSeconds(), 0);
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(1)->getDelayInSeconds(), 2);
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(2)->getDelayInSeconds(), 1.5f);
}

TEST_F(FxTest, ParseActionIgnoreMaster)
{
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(0)->getIgnoreMaster(), false);
    EXPECT_EQ(getFxByName("fx/parserTest1")->getAction(0)->getIgnoreMaster(), true);
}

TEST_F(FxTest, ParseActionShake)
{
    // shake 1.3, 2.7, 0.7 , 1 , 0.33
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(0)->getActionType(), fx::IFxAction::Type::Shake);
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(0)->getShakeTimeInSeconds(), 1.3f);
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(0)->getShakeAmplitude(), 2.7f);
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(0)->getShakeDistance(), 0.7f);
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(0)->getShakeFalloff(), true);
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(0)->getShakeImpulse(), 0.33f);

    // shake 0.0, 2, .7 , 0 , 0.33
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(1)->getActionType(), fx::IFxAction::Type::Shake);
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(1)->getShakeTimeInSeconds(), 0);
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(1)->getShakeAmplitude(), 2.0f);
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(1)->getShakeDistance(), 0.7f);
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(1)->getShakeFalloff(), false);
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(1)->getShakeImpulse(), 0.33f);
}

TEST_F(FxTest, ParseActionNoShadows)
{
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(0)->getNoShadows(), false);
    EXPECT_EQ(getFxByName("fx/parserTest1")->getAction(0)->getNoShadows(), true);
}

TEST_F(FxTest, ParseActionName)
{
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(0)->getName(), "");
    EXPECT_EQ(getFxByName("fx/parserTest/name")->getAction(0)->getName(), "Testaction");
}

TEST_F(FxTest, ParseActionFireSibling)
{
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(0)->getFireSiblingAction(), "");
    EXPECT_EQ(getFxByName("fx/parserTest/sibling")->getAction(0)->getFireSiblingAction(), "SisterAction");
}

TEST_F(FxTest, ParseActionRandomDelay)
{
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(0)->getRandomDelay().first, 0.0f);
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(0)->getRandomDelay().second, 0.0f);
    EXPECT_EQ(getFxByName("fx/parserTest/sibling")->getAction(1)->getRandomDelay().first, 0.9f);
    EXPECT_EQ(getFxByName("fx/parserTest/sibling")->getAction(1)->getRandomDelay().second, 6.888f);
}

TEST_F(FxTest, ParseActionRotate)
{
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(0)->getRotate(), 0.0f);
    EXPECT_EQ(getFxByName("fx/parserTest/sibling")->getAction(1)->getRotate(), 56.7f);
}

TEST_F(FxTest, ParseActionDuration)
{
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(0)->getDurationInSeconds(), 0.5f);
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(1)->getDurationInSeconds(), 0.0f); // not set
}

TEST_F(FxTest, ParseActionTrackOrigin)
{
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(0)->getTrackOrigin(), false);
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(0)->getTrackOrigin(), false); // not set
    EXPECT_EQ(getFxByName("fx/parserTest/sibling")->getAction(1)->getTrackOrigin(), true);
}

TEST_F(FxTest, ParseActionRestart)
{
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(0)->getRestart(), false);
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(0)->getRestart(), false); // not set
    EXPECT_EQ(getFxByName("fx/parserTest/sibling")->getAction(1)->getRestart(), true);
}

TEST_F(FxTest, ParseActionFadeInAndOut)
{
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(0)->getFadeInTimeInSeconds(), 0);
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(0)->getFadeOutTimeInSeconds(), 0);
    EXPECT_EQ(getFxByName("fx/parserTest/fadeIn")->getAction(0)->getFadeInTimeInSeconds(), 1.2f);
    EXPECT_EQ(getFxByName("fx/parserTest/fadeIn")->getAction(1)->getFadeOutTimeInSeconds(), 0.8f);
}

TEST_F(FxTest, ParseActionSize)
{
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(0)->getDecalSize(), 0);
    EXPECT_EQ(getFxByName("fx/parserTest/fadeIn")->getAction(1)->getDecalSize(), 1.5f);
}

TEST_F(FxTest, ParseActionOffset)
{
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(0)->getOffset(), Vector3(0,0,0));
    EXPECT_EQ(getFxByName("fx/parserTest/fadeIn")->getAction(1)->getOffset(), Vector3(1.6f, 0.7f, -0.8f));
}

TEST_F(FxTest, ParseActionAxisAndAngle)
{
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(0)->getAxis(), Vector3(0,0,0));
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(0)->getAngle(), Vector3(0,0,0));

    EXPECT_EQ(getFxByName("fx/parserTest/axisAndAngle")->getAction(0)->getAxis(), Vector3(0.8f, 0.6f, 0.5f));
    EXPECT_EQ(getFxByName("fx/parserTest/axisAndAngle")->getAction(0)->getAngle(), Vector3(0, 0, 0));

    EXPECT_EQ(getFxByName("fx/parserTest/axisAndAngle")->getAction(1)->getAxis(), Vector3(0, 0, 0));
    EXPECT_EQ(getFxByName("fx/parserTest/axisAndAngle")->getAction(1)->getAngle(), Vector3(0.8f, -0.6f, 0.2f));
}

TEST_F(FxTest, ParseActionUseLight)
{
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(0)->getUseLight(), "");

    EXPECT_EQ(getFxByName("fx/parserTest/useLight")->getAction(1)->getUseLight(), "LightOwner");
    // Use light implies that the action is of type light
    EXPECT_EQ(getFxByName("fx/parserTest/useLight")->getAction(1)->getActionType(), fx::IFxAction::Type::Light);
}

TEST_F(FxTest, ParseActionUseModel)
{
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(0)->getUseModel(), "");

    EXPECT_EQ(getFxByName("fx/parserTest/useModel")->getAction(1)->getUseModel(), "ModelOwner");
    // UseModel implies that the action is of type Model
    EXPECT_EQ(getFxByName("fx/parserTest/useModel")->getAction(1)->getActionType(), fx::IFxAction::Type::Model);
}

TEST_F(FxTest, ParseActionAttachLight)
{
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(0)->getAttachLight(), "");

    EXPECT_EQ(getFxByName("fx/parserTest/attach")->getAction(0)->getAttachLight(), "light_1");
    EXPECT_EQ(getFxByName("fx/parserTest/attach")->getAction(0)->getActionType(), fx::IFxAction::Type::AttachLight);
}

TEST_F(FxTest, ParseActionAttachEntity)
{
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(0)->getAttachEntity(), "");

    EXPECT_EQ(getFxByName("fx/parserTest/attach")->getAction(1)->getAttachEntity(), "func_static_1");
    EXPECT_EQ(getFxByName("fx/parserTest/attach")->getAction(1)->getActionType(), fx::IFxAction::Type::AttachEntity);
}

TEST_F(FxTest, ParseActionLaunchProjectile)
{
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(0)->getLaunchProjectileDef(), "");

    EXPECT_EQ(getFxByName("fx/parserTest/projectile")->getAction(0)->getLaunchProjectileDef(), "atdm:projectile_broadhead");
    EXPECT_EQ(getFxByName("fx/parserTest/projectile")->getAction(0)->getActionType(), fx::IFxAction::Type::Launch);
}

TEST_F(FxTest, ParseActionLight)
{
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(0)->getLightMaterialName(), "");
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(0)->getLightRadius(), 0.0f);
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(0)->getLightRgbColour(), Vector3(0,0,0));

    EXPECT_EQ(getFxByName("fx/parserTest/useLight")->getAction(0)->getActionType(), fx::IFxAction::Type::Light);
    EXPECT_EQ(getFxByName("fx/parserTest/useLight")->getAction(0)->getLightMaterialName(), "lights/biground");
    EXPECT_EQ(getFxByName("fx/parserTest/useLight")->getAction(0)->getLightRadius(), 550.3f);
    EXPECT_TRUE(math::isNear(getFxByName("fx/parserTest/useLight")->getAction(0)->getLightRgbColour(), Vector3(0.5, 1, 0.7), 0.001f));
}

TEST_F(FxTest, ParseActionModel)
{
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(0)->getModelName(), "");

    EXPECT_EQ(getFxByName("fx/parserTest/useModel")->getAction(0)->getActionType(), fx::IFxAction::Type::Model);
    EXPECT_EQ(getFxByName("fx/parserTest/useModel")->getAction(0)->getModelName(), "tree.ase");
}

TEST_F(FxTest, ParseActionParticle)
{
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(0)->getModelName(), "");

    EXPECT_EQ(getFxByName("fx/parserTest/useModel")->getAction(2)->getActionType(), fx::IFxAction::Type::Particle);
    EXPECT_EQ(getFxByName("fx/parserTest/useModel")->getAction(2)->getModelName(), "drips.prt");
}

TEST_F(FxTest, ParseActionDecal)
{
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(0)->getDecalMaterialName(), "");

    EXPECT_EQ(getFxByName("fx/parserTest/decal")->getAction(0)->getActionType(), fx::IFxAction::Type::Decal);
    EXPECT_EQ(getFxByName("fx/parserTest/decal")->getAction(0)->getDecalMaterialName(), "textures/decals/blood");
}

TEST_F(FxTest, ParseActionParticleTrackVelocity)
{
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(0)->getParticleTrackVelocity(), false);
    EXPECT_EQ(getFxByName("fx/parserTest/useModel")->getAction(2)->getParticleTrackVelocity(), true);
}

TEST_F(FxTest, ParseActionSound)
{
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(0)->getSoundShaderName(), "");

    EXPECT_EQ(getFxByName("fx/parserTest/sound")->getAction(0)->getActionType(), fx::IFxAction::Type::Sound);
    EXPECT_EQ(getFxByName("fx/parserTest/sound")->getAction(0)->getSoundShaderName(), "footsteps/stone");
}

TEST_F(FxTest, ParseActionShockwave)
{
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(0)->getShockwaveDefName(), "");

    EXPECT_EQ(getFxByName("fx/parserTest/shockwave")->getAction(0)->getActionType(), fx::IFxAction::Type::Shockwave);
    EXPECT_EQ(getFxByName("fx/parserTest/shockwave")->getAction(0)->getShockwaveDefName(), "atdm:some_shockwave_def");
}

}
