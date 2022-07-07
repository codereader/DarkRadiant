#include "gtest/gtest.h"

#include "parser/DefTokeniser.h"
#include "parser/DefBlockTokeniser.h"

namespace test
{

inline void parseBlock(const std::string& testString,
    const std::vector<std::pair<std::string, std::string>>& expectedBlocks)
{
    std::istringstream stream{ testString };
    parser::BasicDefBlockTokeniser<std::istream> tokeniser(stream);

    for (const auto& expectedBlock : expectedBlocks)
    {
        EXPECT_TRUE(tokeniser.hasMoreBlocks());

        auto block = tokeniser.nextBlock();
        EXPECT_EQ(block.name, expectedBlock.first);
        EXPECT_NE(block.contents.find(expectedBlock.second), std::string::npos);
    }
}

inline void parseBlock(const std::string& testString, 
    const std::string& expectedName, const std::string& needleToFindInBlockContents)
{
    parseBlock(testString, { std::make_pair(expectedName, needleToFindInBlockContents) });
}

TEST(DefBlockTokeniser, TypedBlock)
{
    std::string testString = R"(entityDef atdm:target_base
{
	"inherit"				"atdm:entity_base"

	"editor_displayFolder"	"Targets"
	"nodraw"				"1"
})";

    parseBlock(testString, "entityDef atdm:target_base", "\"atdm:entity_base\"");
}

TEST(DefBlockTokeniser, TypedBlockWithWhitespace)
{
    std::string testString = R"(	entityDef	atdm:target_base  		
{
	"inherit"				"atdm:entity_base"

	"editor_displayFolder"	"Inner opening { and closing } braces to confuse the parser"
	"nodraw"				"1"
})";

    parseBlock(testString, "entityDef atdm:target_base", "\"atdm:entity_base\"");
}

TEST(DefBlockTokeniser, TypedBlockWithCurlyBracesInContent)
{
    std::string testString = R"(	entityDef	atdm:target_base  		
{
	"editor_displayFolder"	"Inner opening { and closing } braces to confuse the parser"
	"nodraw"				"1"
})";

    parseBlock(testString, "entityDef atdm:target_base", "Inner opening { and closing } braces to confuse the parser");
}

TEST(DefBlockTokeniser, NestedBlock)
{
    std::string testString = R"(textures/darkmod/test  		
{
    {
        blend diffusemap
        map _white
    }
})";

    parseBlock(testString, "textures/darkmod/test", "blend diffusemap");
}

TEST(DefBlockTokeniser, NameAndBlockInSeparateLines)
{
    std::string testString = R"(textures/parsing_test/variant1
    {
        diffusemap _white
    })";

    parseBlock(testString, "textures/parsing_test/variant1", "_white");
}

TEST(DefBlockTokeniser, CommentAfterBlockName)
{
    std::string testString = R"(textures/parsing_test/commentAfterBlockName // comment separated by whitespace
    {
        diffusemap _white
    })";

    parseBlock(testString, "textures/parsing_test/commentAfterBlockName", "_white");
}

TEST(DefBlockTokeniser, CommentAfterBlockNameNoWhitespace)
{
    std::string testString = R"(textures/parsing_test/CommentAfterBlockNameNoWhitespace// comment without whitespace
    {
        diffusemap _white
    })";

    parseBlock(testString, "textures/parsing_test/CommentAfterBlockNameNoWhitespace", "_white");
}

TEST(DefBlockTokeniser, CommentInBetweenNameAndBlock)
{
    std::string testString = R"(textures/parsing_test/commentInBetweenNameAndBlock
// comment between name and block
{
    diffusemap _white
})";

    parseBlock(testString, "textures/parsing_test/commentInBetweenNameAndBlock", "_white");
}

TEST(DefBlockTokeniser, NameAndBlockInSameLine)
{
    std::string testString = R"(textures/parsing_test/variant2 {
        diffusemap _white
    })";

    parseBlock(testString, "textures/parsing_test/variant2", "_white");
}

TEST(DefBlockTokeniser, NameAndBlockInSameLineNoWhitespace)
{
    std::string testString = R"(textures/parsing_test/variant3{
        diffusemap _white
    })";

    parseBlock(testString, "textures/parsing_test/variant3", "_white");
}

TEST(DefBlockTokeniser, BlockSeparation)
{
    std::string testString = R"(textures/parsing_test/block1 {
        diffusemap _white
    }

    textures/parsing_test/block2 {
        diffusemap _white
    } textures/parsing_test/block3 {
        diffusemap _white
    }
    textures/parsing_test/block4 {
        diffusemap _white
    }textures/parsing_test/block5 {
        diffusemap _white
    })";

    parseBlock(testString, {
        std::make_pair("textures/parsing_test/block1", "_white"),
        std::make_pair("textures/parsing_test/block2", "_white"),
        std::make_pair("textures/parsing_test/block3", "_white"),
        std::make_pair("textures/parsing_test/block4", "_white"),
        std::make_pair("textures/parsing_test/block5", "_white"),
    });
}

// The tokeniser normalises the whitespace between type and name to a single space
TEST(DefBlockTokeniser, WhitespaceAfterTypename)
{
    std::string testString = R"(textures/parsing_test/block1 {
        diffusemap _white
    }

    // Add a tab in between type name and block name
    sound	textures/parsing_test/block1a {
        diffusemap _white
    }

    // spaces after the typename, and some tabs after the name
    sound    textures/parsing_test/block1b		{
        diffusemap _white
    }

    // Tabs before and after the name
    sound	  	  	textures/parsing_test/block1c
    {
        diffusemap _white
    }

    textures/parsing_test/block2 {
        diffusemap _white
    } sound textures/parsing_test/block3 {
        diffusemap _white
    }
    sound textures/parsing_test/block4 {
        diffusemap _white
    }sound textures/parsing_test/block5 {
        diffusemap _white
    })";

    parseBlock(testString, {
        std::make_pair("textures/parsing_test/block1", "_white"),
        std::make_pair("sound textures/parsing_test/block1a", "_white"),
        std::make_pair("sound textures/parsing_test/block1b", "_white"),
        std::make_pair("sound textures/parsing_test/block1c", "_white"),
        std::make_pair("textures/parsing_test/block2", "_white"),
        std::make_pair("sound textures/parsing_test/block3", "_white"),
        std::make_pair("sound textures/parsing_test/block4", "_white"),
        std::make_pair("sound textures/parsing_test/block5", "_white"),
    });
}

TEST(DefTokeniser, EmptyQuotesAtEndOfBlock)
{
    std::string testString = R"(
	"inherit"					"atdm:mover_handle_base"

	"spawnclass"				"CFrobLockHandle"

	"editor_DisplayFolder"		"Movers"
	"editor_usage"				"Attach to a lock by binding it."

	"editor_snd snd_tap_locked"		"Called when the handle starts to move and its door is locked."
	"editor_snd snd_tap_default"	"Called when the handle starts to move and its door is unlocked."

    "noclipmodel"               "1"

    "mins"                      "-1 -1 -3"
    "maxs"                      "1 1 3"
    "frobbox_size"              "1 1 1"

	"snd_tap_default"			""
	"snd_tap_locked"			""
)";

    parser::BasicDefTokeniser<std::string> tokeniser(testString);

    std::map<std::string, std::string> keyValuePairs;
    while (tokeniser.hasMoreTokens())
    {
        auto key = tokeniser.nextToken();
        auto value = tokeniser.nextToken();

        keyValuePairs.emplace(std::move(key), std::move(value));
    }

    EXPECT_EQ(keyValuePairs.size(), 12) << "Expected 12 key value pairs after parsing";

    EXPECT_EQ(keyValuePairs["snd_tap_locked"], "");
    EXPECT_EQ(keyValuePairs["editor_snd snd_tap_locked"], "Called when the handle starts to move and its door is locked.");
    EXPECT_EQ(keyValuePairs["mins"], "-1 -1 -3");
}

}
