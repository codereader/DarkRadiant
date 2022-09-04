#include "ifx.h"

#include "RadiantTest.h"

namespace test
{

using FxTest = RadiantTest;

TEST_F(FxTest, GetFxByName)
{
    EXPECT_TRUE(GlobalDeclarationManager().findDeclaration(decl::Type::Fx, "fx/tdm_flame"));
    EXPECT_TRUE(GlobalDeclarationManager().findDeclaration(decl::Type::Fx, "fx/sparks"));
}

}
