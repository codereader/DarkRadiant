#include "RadiantTest.h"

#include "parser/CodeTokeniser.h"

#include "testutil/TemporaryFile.h"

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

constexpr const char* const TEMPORARY_GUI_FILE = "guis/temporary.gui";

inline void expectTokenSequence(const radiant::TestContext& context, const std::string& contents, std::vector<std::string> expectedTokens)
{
    auto path = context.getTestProjectPath() + TEMPORARY_GUI_FILE;
    TemporaryFile tempFile(path, contents);

    auto file = GlobalFileSystem().openTextFile(TEMPORARY_GUI_FILE);
    parser::CodeTokeniser tokeniser(file);

    expectTokenSequence(tokeniser, expectedTokens);
}

TEST_F(CodeTokeniser, ParseWindowDef)
{
    std::string contents = R"(
    windowDef rightPageBackground {
		rect -100, -10, 500, 400
		background "guis/assets/background_right_01"
		matcolor 1, 1, 1, 0
	})";

    expectTokenSequence(_context, contents,
    {
        "windowDef", "rightPageBackground",
        "{",
            "rect", "-100",",","-10",",","500",",","400",
            "background", "guis/assets/background_right_01",
            "matcolor", "1",",","1",",","1",",","0",
        "}",
    });
}

// The game's GUI parser supports reading rect-100 values without whitespace separation
TEST_F(CodeTokeniser, ParseRectExpression)
{
    std::string contents = R"(
    windowDef rightPageBackground {
		rect-100,-10, 500, 400
	})";

    expectTokenSequence(_context, contents,
    {
        "windowDef", "rightPageBackground",
        "{",
            "rect", "-100",",","-10",",","500",",","400",
        "}",
    });
}

TEST_F(CodeTokeniser, ParseOnTimeExpression)
{
    std::string contents = R"(
    onTime 0 {
        if ("gui::worldDisplay" == 1)
        {
            set "title::forecolor" "0 0 0 1";
            set "body::forecolor" "0 0 0 1";
        }
        set "title::text" "$gui::title";
    })";

    expectTokenSequence(_context, contents,
    {
        "onTime", "0",
        "{",
            "if", "(", "gui::worldDisplay","==","1",")",
            "{",
                "set", "title::forecolor", "0 0 0 1", ";",
                "set", "body::forecolor", "0 0 0 1", ";",
            "}",
            "set", "title::text", "$gui::title", ";"
        "}",
    });
}

TEST_F(CodeTokeniser, PreprocessorDirectives)
{
    // The file contains a couple of preprocessor expressions
    auto file = GlobalFileSystem().openTextFile("guis/parse_test2.gui");
    parser::CodeTokeniser tokeniser(file);

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
