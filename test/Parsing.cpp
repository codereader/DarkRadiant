#include "gtest/gtest.h"

#include "parser/DefBlockTokeniser.h"

namespace test
{

TEST(DefBlockTokeniser, NameAndBlockInSeparateLines)
{
    std::string testString = R"(textures/parsing_test/variant1
    {
        diffusemap _white
    })";

    std::istringstream stream{testString};

    parser::BasicDefBlockTokeniser<std::istream> tokeniser(stream);

    auto block = tokeniser.nextBlock();
    EXPECT_EQ(block.name, "textures/parsing_test/variant1");
    EXPECT_NE(block.contents.find("_white"), std::string::npos);
}

}
