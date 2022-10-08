#include "RadiantTest.h"

#include "parser/CodeTokeniser.h"

namespace test
{

using CodeTokeniser = RadiantTest;

inline void expectTokenSequence(parser::CodeTokeniser& tokeniser, std::vector<std::string> expectedTokens)
{
    for (auto expectedToken = expectedTokens.begin(); expectedToken != expectedTokens.end(); ++expectedToken)
    {
        EXPECT_TRUE(tokeniser.hasMoreTokens()) << "Tokeniser ran out of tokens";
        EXPECT_EQ(tokeniser.nextToken(), *expectedToken);
    }

    EXPECT_FALSE(tokeniser.hasMoreTokens()) << "Tokeniser should have no more tokens now";
}

TEST_F(CodeTokeniser, PreprocessorDirectives)
{
    // The file contains a couple of preprocessor expressions
    auto file = GlobalFileSystem().openTextFile("guis/parse_test2.gui");
    parser::CodeTokeniser tokeniser(file, parser::WHITESPACE, "{}(),;");

    expectTokenSequence(tokeniser,
    {
        "windowDef", "Contents",
        "{",
            "forceaspectwidth", "633",
            "forceaspectheight", "211",
            "windowDef", "IncludedWindow",
            "{",
                "rect", "0",",","0",",","640",",","480",
            "}",
        "}",
        "windowDef", "ThisShouldAppear", "{", "}",
        "windowDef", "ThisShouldAppearAsWell", "{", "}"
    });
}

}
