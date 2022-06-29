#include "RadiantTest.h"

namespace test
{

using CommandSystemTest = RadiantTest;

TEST_F(CommandSystemTest, GetCommandSystem)
{
    const auto& mod = GlobalCommandSystem();
    EXPECT_EQ(mod.getName(), "CommandSystem");
}

}