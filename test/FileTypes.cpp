#include "RadiantTest.h"

#include "ifiletypes.h"

namespace test
{

using FileTypesTest = RadiantTest;

TEST_F(FileTypesTest, WildcardRegistered)
{
    auto patterns = GlobalFiletypes().getPatternsForType("*");

    ASSERT_EQ(patterns.size(), 1);
    ASSERT_EQ(patterns.front().extension, "*");
}

TEST_F(FileTypesTest, Registration)
{
    auto customType = "_custom";
    auto anotherType = "_custom2";

    // Register two sets of patterns, one for each of the custom types
    GlobalFiletypes().registerPattern(customType, FileTypePattern("Name1", "ext1", "*.ext1"));
    GlobalFiletypes().registerPattern(customType, FileTypePattern("Name2", "ext2", "*.ext1"));

    GlobalFiletypes().registerPattern(anotherType, FileTypePattern("Name1", "ext1", "*.ext1"));
    GlobalFiletypes().registerPattern(anotherType, FileTypePattern("Name2", "ext2", "*.ext2"));

    // Retrieve the patterns for one file type
    auto patterns = GlobalFiletypes().getPatternsForType(customType);

    // The registration for anotherType should not interfere with the query
    ASSERT_EQ(patterns.size(), 2);
    ASSERT_NE(std::find_if(patterns.begin(), patterns.end(), [](const FileTypePattern& p) { return p.extension == "ext1"; }), patterns.end());
    ASSERT_NE(std::find_if(patterns.begin(), patterns.end(), [](const FileTypePattern& p) { return p.extension == "ext2"; }), patterns.end());
}

TEST_F(FileTypesTest, IconRegistration)
{
    auto customType = "_custom";
    auto anotherType = "_custom2";
    auto icon1 = "icon1.png";
    auto icon2 = "icon2.png";

    // Register two sets of patterns, one for each of the custom types
    GlobalFiletypes().registerPattern(customType, FileTypePattern("Name1", "ext1", "*.ext1", icon1));
    GlobalFiletypes().registerPattern(customType, FileTypePattern("Name2", "ext2", "*.ext1", icon2));

    // Here a different icon for the same extension is registered
    GlobalFiletypes().registerPattern(anotherType, FileTypePattern("Name1", "ext1", "*.ext1", icon2)); 
    GlobalFiletypes().registerPattern(anotherType, FileTypePattern("Name2", "ext2", "*.ext2"));

    // Check if an icon can be found
    auto foundIcon = GlobalFiletypes().getIconForExtension("ext1");

    // It's not defined which icon will be returned if two patterns define an icon
    ASSERT_TRUE(foundIcon == icon1 || foundIcon == icon2);

    // For ext2 there is only one icon registered
    ASSERT_TRUE(GlobalFiletypes().getIconForExtension("ext2") == icon2);

    // Query an unregistered extension
    ASSERT_TRUE(GlobalFiletypes().getIconForExtension("no_ext_like_this") == std::string());
}

}
