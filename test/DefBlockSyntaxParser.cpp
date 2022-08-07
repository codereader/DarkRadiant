#include "gtest/gtest.h"

#include "parser/DefBlockSyntaxParser.h"

namespace test
{

TEST(DefBlockSyntaxParser, EmptyText)
{
    std::string testString = "";

    parser::DefBlockSyntaxParser<std::string> parser(testString);
    auto root = parser.parse();

    EXPECT_TRUE(root) << "Syntax Root must not be null";
}

}
