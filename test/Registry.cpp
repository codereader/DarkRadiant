#include "RadiantTest.h"

#include "registry/registry.h"

namespace test
{

using RegistryTest = RadiantTest;

TEST_F(RegistryTest, GetValue)
{
    // These keys exist in the bundled user.xml so should exist regardless of user settings
    EXPECT_EQ(registry::getValue<int>("user/ui/map/numMRU"), 5);
    EXPECT_EQ(registry::getValue<bool>("user/ui/map/autoSaveEnabled"), true);
    EXPECT_EQ(registry::getValue<float>("user/ui/ModelSelector/previewSizeFactor"), 0.7f);
    EXPECT_EQ(
        registry::getValue<std::string>("user/ui/manipulatorFontStyle"), std::string("Sans")
    );
}

}
