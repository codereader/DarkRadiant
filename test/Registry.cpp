#include "RadiantTest.h"

#include "registry/registry.h"

namespace test
{

using RegistryTest = RadiantTest;

TEST_F(RegistryTest, GetTypedValue)
{
    // These keys exist in the bundled user.xml so should exist regardless of user settings
    EXPECT_EQ(registry::getValue<int>("user/ui/map/numMRU"), 5);
    EXPECT_EQ(registry::getValue<bool>("user/ui/map/autoSaveEnabled"), true);
    EXPECT_EQ(registry::getValue<float>("user/ui/ModelSelector/previewSizeFactor"), 0.7f);
    EXPECT_EQ(
        registry::getValue<std::string>("user/ui/manipulatorFontStyle"), std::string("Sans")
    );
}

namespace
{
    xml::Node nodeFromPath(const std::string& path)
    {
        auto list = GlobalRegistry().findXPath(path);
        return list.at(0);
    }
}

TEST_F(RegistryTest, SetTypedValue)
{
    const char* RKEY_INTVALUE = "user/test/intValue";
    const char* RKEY_BOOLVALUE = "user/test/boolValue";
    const char* RKEY_FLOATVALUE = "user/test/floatValue";
    const char* RKEY_STRINGVALUE = "user/test/stringValue";

    registry::setValue(RKEY_INTVALUE, 57);
    registry::setValue(RKEY_BOOLVALUE, true);
    registry::setValue(RKEY_FLOATVALUE, 1.374f);
    registry::setValue(RKEY_STRINGVALUE, std::string("whatever"));

    EXPECT_EQ(registry::getValue<int>(RKEY_INTVALUE), 57);
    EXPECT_EQ(registry::getValue<bool>(RKEY_BOOLVALUE), true);
    EXPECT_EQ(registry::getValue<float>(RKEY_FLOATVALUE), 1.374f);
    EXPECT_EQ(registry::getValue<std::string>(RKEY_STRINGVALUE), "whatever");

    // Look up the nodes themselves
    const auto nodeHasContentOnly = [](const char* key, const char* expValue) {
        const auto node = nodeFromPath(key);
        EXPECT_EQ(node.getContent(), expValue);
        EXPECT_EQ(node.getAttributeValue("value"), "");
    };
    nodeHasContentOnly(RKEY_INTVALUE, "57");
    nodeHasContentOnly(RKEY_BOOLVALUE, "1");
    nodeHasContentOnly(RKEY_FLOATVALUE, "1.374000");
    nodeHasContentOnly(RKEY_STRINGVALUE, "whatever");
}

TEST_F(RegistryTest, ReadOrRemoveLegacyValueAttribute)
{
    const char* INTKEY = "user/test/intValue";
    const char* BOOLKEY = "user/test/boolValue";

    // Manually create the nodes with a legacy "value" attribute
    GlobalRegistry().createKey(INTKEY).setAttributeValue("value", "37");
    GlobalRegistry().createKey(BOOLKEY).setAttributeValue("value", "1");

    // The value attribute should be readable as a registry key
    EXPECT_EQ(registry::getValue<int>(INTKEY), 37);
    EXPECT_EQ(registry::getValue<bool>(BOOLKEY), true);

    // Write using the standard API
    registry::setValue(INTKEY, 27);
    registry::setValue(BOOLKEY, false);

    // Value attributes should have disappeared
    EXPECT_EQ(GlobalRegistry().findXPath(INTKEY).at(0).getAttributeValue("value"), "");
    EXPECT_EQ(GlobalRegistry().findXPath(BOOLKEY).at(0).getAttributeValue("value"), "");

    // Read back using the standard API
    EXPECT_EQ(registry::getValue<int>(INTKEY), 27);
    EXPECT_EQ(registry::getValue<bool>(BOOLKEY), false);
}

TEST_F(RegistryTest, CreateNodeWithContent)
{
    const char* KEY = "user/test/nodes/nonExistent";

    EXPECT_FALSE(GlobalRegistry().keyExists(KEY));
    auto node = GlobalRegistry().createKey(KEY);
    EXPECT_TRUE(GlobalRegistry().keyExists(KEY));

    // Set some node content
    EXPECT_EQ(node.getContent(), "");
    node.setContent("Node content");
    EXPECT_EQ(node.getContent(), "Node content");
}

TEST_F(RegistryTest, SetAttributeOnNode)
{
    const char* KEY = "user/test/anotherNode";

    auto node = GlobalRegistry().createKey(KEY);
    node.setAttributeValue("FirstAttribute", "Value1");
    node.setAttributeValue("SecondAttribute", "Value2");

    EXPECT_EQ(node.getAttributeValue("FirstAttribute"), "Value1");
    EXPECT_EQ(node.getAttributeValue("SecondAttribute"), "Value2");

    // Attributes do not affect content
    EXPECT_EQ(node.getContent(), "");
}

}
