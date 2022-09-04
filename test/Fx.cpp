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
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(2)->getDelay(), 1.5);
}

TEST_F(FxTest, ParseActionIgnoreMaster)
{
    EXPECT_EQ(getFxByName("fx/sparks")->getAction(0)->getIgnoreMaster(), false);
    EXPECT_EQ(getFxByName("fx/parserTest1")->getAction(0)->getIgnoreMaster(), true);
}

}
