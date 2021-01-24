#include "RadiantTest.h"

#include "ieclass.h"

namespace test
{

using EntityTest = RadiantTest;

TEST_F(EntityTest, LookupEntityClass)
{
    // Nonexistent class should return null (but not throw or crash)
    auto cls = GlobalEntityClassManager().findClass("notAnEntityClass");
    EXPECT_FALSE(cls);

    // Real entity class should return a valid pointer
    auto lightCls = GlobalEntityClassManager().findClass("light");
    EXPECT_TRUE(lightCls);
}

TEST_F(EntityTest, LightClassRecognisedAsLight)
{
    // The 'light' class should be recognised as an actual light
    auto lightCls = GlobalEntityClassManager().findClass("light");
    EXPECT_TRUE(lightCls->isLight());

    // Things which are not lights should also be correctly identified
    auto notLightCls = GlobalEntityClassManager().findClass("dr:entity_using_modeldef");
    EXPECT_TRUE(notLightCls);
    EXPECT_FALSE(notLightCls->isLight());
}

}