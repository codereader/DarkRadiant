#include "RadiantTest.h"

#include "parser/CodeTokeniser.h"
#include "parser/GuiTokeniser.h"

#include "testutil/TemporaryFile.h"

namespace test
{

using GuiTokeniser = RadiantTest;

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

    parser::GuiTokeniser tokeniser(file);
    expectTokenSequence(tokeniser, expectedTokens);
}

TEST_F(GuiTokeniser, ParseWindowDef)
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
            "rect", "-","100",",","-", "10",",","500",",","400",
            "background", "guis/assets/background_right_01",
            "matcolor", "1",",","1",",","1",",","0",
        "}",
    });
}

// The game's GUI parser supports reading rect-100 values without whitespace separation
TEST_F(GuiTokeniser, ParseRectExpression)
{
    std::string contents = R"(
    windowDef rightPageBackground {
		rect-100,-10, 500, 400
	})";

    expectTokenSequence(_context, contents,
    {
        "windowDef", "rightPageBackground",
        "{",
            "rect", "-","100",",","-","10",",","500",",","400",
        "}",
    });
}

TEST_F(GuiTokeniser, ParseOnTimeExpression)
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
            "set", "title::text", "$gui::title", ";",
        "}",
    });
}

TEST_F(GuiTokeniser, ParseQuotes)
{
    std::string contents = R"(
    // Escaped double quotes
    if ("gui::\"worldDisplay\"" == 1)
    {
        // Single quotes
        set 'title::forecolor' "0 0 0 1";
        // Single quotes containing double quotes
        set 'body::"forecolor"' "0 0 0 1";
    }
    // Single quotes with escaped single quotes
    set 'that\'s something' "$gui::title";
    )";

    expectTokenSequence(_context, contents,
    {
        "if", "(", "gui::\"worldDisplay\"","==","1",")",
        "{",
            "set", "title::forecolor", "0 0 0 1", ";",
            "set", "body::\"forecolor\"", "0 0 0 1", ";",
        "}",
        "set", "that's something", "$gui::title", ";",
    });
}

TEST_F(GuiTokeniser, ParseComparisonOperators)
{
    expectTokenSequence(_context, R"("visible" > 1)", { "visible", ">", "1" });
    expectTokenSequence(_context, R"("visible" < 1)", { "visible", "<", "1" });
    expectTokenSequence(_context, R"("visible" >= 1)", { "visible", ">=", "1" });
    expectTokenSequence(_context, R"("visible" <= 1)", { "visible", "<=", "1" });
    expectTokenSequence(_context, R"("visible" != 1)", { "visible", "!=", "1" });
    expectTokenSequence(_context, R"("visible" == 1)", { "visible", "==", "1" });

    // Without whitespace
    expectTokenSequence(_context, R"("visible">1)", { "visible", ">", "1" });
    expectTokenSequence(_context, R"("visible"<1)", { "visible", "<", "1" });
    expectTokenSequence(_context, R"("visible">=1)", { "visible", ">=", "1" });
    expectTokenSequence(_context, R"("visible"<=1)", { "visible", "<=", "1" });
    expectTokenSequence(_context, R"("visible"!=1)", { "visible", "!=", "1" });
    expectTokenSequence(_context, R"("visible"==1)", { "visible", "==", "1" });

    // This is semantic garbage but the parser should pick something up
    expectTokenSequence(_context, R"("visible">>1)", { "visible", ">", ">", "1" });
    expectTokenSequence(_context, R"("visible" > >1)", { "visible", ">", ">", "1" });
    expectTokenSequence(_context, R"("visible" > > 1)", { "visible", ">", ">", "1" });
    expectTokenSequence(_context, R"("visible" <>= 1)", { "visible", "<", ">=", "1" });
    expectTokenSequence(_context, R"("visible"<>=1)", { "visible", "<", ">=", "1" });

    // With some parentheses around it
    expectTokenSequence(_context, R"(("visible"==1))", { "(", "visible", "==", "1", ")" });
}

TEST_F(GuiTokeniser, ParseMathOperators)
{
    expectTokenSequence(_context, R"("width"*3)", { "width", "*", "3" });
    expectTokenSequence(_context, R"("width"*-3)", { "width", "*", "-", "3" });
    expectTokenSequence(_context, R"(3*width+4)", { "3", "*", "width", "+", "4" });
    expectTokenSequence(_context, R"(3%width+4)", { "3", "%", "width", "+", "4" });
    expectTokenSequence(_context, R"("width" - -3)", { "width", "-", "-", "3" });
    expectTokenSequence(_context, R"('gui::test' - (-3+7.1))", { "gui::test", "-", "(", "-", "3", "+", "7.1", ")" });
    expectTokenSequence(_context, R"(3||6+4)", { "3", "||", "6", "+", "4" });
    expectTokenSequence(_context, R"(3||6&&4)", { "3", "||", "6", "&&", "4" });
    expectTokenSequence(_context, R"(3 || -6 && 4)", { "3", "||", "-", "6", "&&", "4" });
    expectTokenSequence(_context, R"(3 < 6.4)", { "3", "<", "6.4" });
    expectTokenSequence(_context, R"(3<6.4)", { "3", "<", "6.4" });
    expectTokenSequence(_context, R"(3/5)", { "3", "/", "5" });
    expectTokenSequence(_context, R"(3/-5)", { "3", "/", "-", "5" });
    expectTokenSequence(_context, R"(3/5)", { "3", "/", "5" });
    expectTokenSequence(_context, R"("visible" = 3 > 0)", { "visible", "=", "3", ">", "0" });
    expectTokenSequence(_context, R"(rect  ("gui::ingame" ? 0 : 50), -75, 640, 640)",
        { "rect","(","gui::ingame","?","0",":","50",")",",","-", "75",",","640",",","640" });
    expectTokenSequence(_context, R"(rect  ("gui::ingame"?0:50), -75, 640, 640)",
        { "rect","(","gui::ingame","?","0",":","50",")",",","-","75",",","640",",","640" });
    expectTokenSequence(_context, R"("gui::av_in_progress" == 0 && "exit" == 0)",
        { "gui::av_in_progress","==","0","&&","exit","==","0" });
}

TEST_F(GuiTokeniser, ParseMacroExpansion)
{
    std::string contents = R"(
    #define POS_START_X 40
    #define SIZE_MULTIPLIER 1
    #define SUB_WINDOW(Name, Type, VisibleCheck) Type Name \
    {\
        rect POS_START_X*SIZE_MULTIPLIER,50,60,70\
        visible VisibleCheck\
    }

    #define fullWindow(parent, start, objvisible, box, objtext)\
    windowDef parent\
	{\
		rect		POS_START_X, start, 490, 16\
		visible		objvisible\
        SUB_WINDOW(box, windowDef, objvisible)\
		text		objtext\
	}
	#define exampleText "#str_02925"
	fullWindow(Obj_t1_parent, 10, "gui::NumObjectivesPerPage" >= 1, Objbox_t1, exampleText)
    )";

    expectTokenSequence(_context, contents,
    {
        "windowDef", "Obj_t1_parent",
        "{",
            "rect", "40",",","10",",","490",",","16",
            "visible","gui::NumObjectivesPerPage",">=", "1",
            "windowDef", "Objbox_t1",
            "{",
                "rect","40","*","1",",","50",",","60",",","70",
                "visible","gui::NumObjectivesPerPage",">=", "1",
            "}",
            "text","#str_02925",
        "}",
    });
}

TEST_F(GuiTokeniser, ParseObjectivesMacroExpansion)
{
    // Taken from TDM's objectives menu
    std::string contents = R"(
    #define POS_START_X 40
    #define SIZE_MULTIPLIER 1
    #define POS_OBJ1_START_Y 10

    #define objectiveExample(parent, start, objvisible, box, obj, objtext)\
    windowDef parent\
	{\
		rect		POS_START_X, start, 490, 16\
		visible		objvisible\
		windowDef box\
		{\
			rect		0, -5, 32*SIZE_MULTIPLIER, 32*SIZE_MULTIPLIER\
			bordercolor	1,1,1,1\
			visible		1}\
		windowDef obj\
		{\
			rect		25*SIZE_MULTIPLIER, 0, 430, 60*SIZE_MULTIPLIER*SIZE_MULTIPLIER\
			bordercolor	1,1,1,1\
			text		objtext\
			visible		1\
		}\
	}
	#define exampleText "#str_02925" // This is an example text to illustrate the size of the objectives text.
	objectiveExample(Obj_t1_parent, POS_OBJ1_START_Y, "gui::NumObjectivesPerPage" >= 1, Objbox_t1, Obj_t1, exampleText)
    )";

    expectTokenSequence(_context, contents,
    {
        "windowDef", "Obj_t1_parent",
        "{",
            "rect", "40",",","10",",","490",",","16",
            "visible","gui::NumObjectivesPerPage",">=", "1",
            "windowDef", "Objbox_t1",
            "{",
                "rect","0",",","-","5",",","32","*","1",",","32","*","1",
                "bordercolor","1",",","1",",","1",",","1",
                "visible","1",
            "}",
            "windowDef", "Obj_t1",
            "{",
                "rect","25","*","1",",","0",",","430",",","60","*","1","*","1",
                "bordercolor","1",",","1",",","1",",","1",
                "text","#str_02925",
                "visible","1",
            "}",
        "}",
    });
}

TEST_F(GuiTokeniser, PreprocessorDirectives)
{
    // The file contains a couple of preprocessor expressions
    auto file = GlobalFileSystem().openTextFile("guis/parse_test2.gui");
    parser::GuiTokeniser tokeniser(file);

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
