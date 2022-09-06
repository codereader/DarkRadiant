#include "ifx.h"

#include "RadiantTest.h"

namespace test
{

using FxTest = RadiantTest;

inline fx::IFxDeclaration::Ptr getFxByName(const std::string& name)
{
    return std::static_pointer_cast<fx::IFxDeclaration>(
        GlobalDeclarationManager().findDeclaration(decl::Type::Fx, name));
}

TEST_F(FxTest, GetFxByName)
{
    EXPECT_TRUE(GlobalDeclarationManager().findDeclaration(decl::Type::Fx, "fx/tdm_flame"));
    EXPECT_TRUE(GlobalDeclarationManager().findDeclaration(decl::Type::Fx, "fx/sparks"));
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
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(0)->getDelay(), 0);
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(1)->getDelay(), 2);
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(2)->getDelay(), 1.5f);
}

TEST_F(FxTest, ParseActionIgnoreMaster)
{
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(0)->getIgnoreMaster(), false);
    EXPECT_EQ(getFxByName("fx/parserTest1")->getAction(0)->getIgnoreMaster(), true);
}

TEST_F(FxTest, ParseActionShake)
{
    // shake 1.3, 2.7, 0.7 , 1 , 0.33
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(0)->getType(), fx::IFxAction::Type::Shake);
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(0)->getShakeTime(), 1.3f);
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(0)->getShakeAmplitude(), 2.7f);
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(0)->getShakeDistance(), 0.7f);
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(0)->getShakeFalloff(), true);
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(0)->getShakeImpulse(), 0.33f);

    // shake 0.0, 2, .7 , 0 , 0.33
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(1)->getType(), fx::IFxAction::Type::Shake);
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(1)->getShakeTime(), 0);
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
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(0)->getDuration(), 0.5f);
    EXPECT_EQ(getFxByName("fx/parserTest/shake")->getAction(1)->getDuration(), 0.0f); // not set
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
    EXPECT_EQ(getFxByName("fx/parserTest/useLight")->getAction(1)->getType(), fx::IFxAction::Type::Light);
}

TEST_F(FxTest, ParseActionAttachLight)
{
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(0)->getAttachLight(), "");

    EXPECT_EQ(getFxByName("fx/parserTest/attach")->getAction(0)->getAttachLight(), "light_1");
    EXPECT_EQ(getFxByName("fx/parserTest/attach")->getAction(0)->getType(), fx::IFxAction::Type::AttachLight);
}

TEST_F(FxTest, ParseActionAttachEntity)
{
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(0)->getAttachEntity(), "");

    EXPECT_EQ(getFxByName("fx/parserTest/attach")->getAction(1)->getAttachEntity(), "func_static_1");
    EXPECT_EQ(getFxByName("fx/parserTest/attach")->getAction(1)->getType(), fx::IFxAction::Type::AttachEntity);
}

}
