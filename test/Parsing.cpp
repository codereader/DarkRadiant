#include "gtest/gtest.h"

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

}
