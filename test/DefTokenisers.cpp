#include "gtest/gtest.h"

#include "parser/DefTokeniser.h"

namespace test
{

inline std::map<std::string, std::string> parseStringPairs(parser::DefTokeniser& tokeniser)
{
    std::map<std::string, std::string> pairs;

    while (tokeniser.hasMoreTokens())
    {
        auto key = tokeniser.nextToken();
        auto value = tokeniser.nextToken();

        pairs.emplace(std::move(key), std::move(value));
    }

    return pairs;
}

TEST(DefTokeniser, ParseEmptyString)
{
    std::string testString = "";
    parser::BasicDefTokeniser<std::string> tokeniser(testString);

    // Tokeniser should be exhausted right after construction
    EXPECT_FALSE(tokeniser.hasMoreTokens());
}

TEST(DefTokeniser, ParseJustWhitespace)
{
    std::string testString = " \t \r\n\t";
    parser::BasicDefTokeniser<std::string> tokeniser(testString);

    // Tokeniser should be exhausted right after construction
    EXPECT_FALSE(tokeniser.hasMoreTokens());
}

TEST(DefTokeniser, SinglePair)
{
    std::string testString = R"("inherit"					"atdm:mover_handle_base")";
    parser::BasicDefTokeniser<std::string> tokeniser(testString);

    auto keyValuePairs = parseStringPairs(tokeniser);
    EXPECT_EQ(keyValuePairs.size(), 1) << "Expected 1 key value pair after parsing";

    EXPECT_EQ(keyValuePairs["inherit"], "atdm:mover_handle_base");
}

TEST(DefTokeniser, SingleEmptyQuote)
{
    std::string testString = R"("")";
    parser::BasicDefTokeniser<std::string> tokeniser(testString);

    // We should receive a single empty string
    EXPECT_TRUE(tokeniser.hasMoreTokens());
    EXPECT_EQ(tokeniser.nextToken(), "");
    EXPECT_FALSE(tokeniser.hasMoreTokens());

    testString = R"(		"" )";
    parser::BasicDefTokeniser<std::string> tokeniser2(testString);

    // We should receive a single empty string
    EXPECT_TRUE(tokeniser2.hasMoreTokens());
    EXPECT_EQ(tokeniser2.nextToken(), "");
    EXPECT_FALSE(tokeniser2.hasMoreTokens());
}

TEST(DefTokeniser, StringContinuation)
{
    std::string testString = R"( "inherit"	"atdm:" \
    "mover_handle_base")";

    parser::BasicDefTokeniser<std::string> tokeniser(testString);

    auto keyValuePairs = parseStringPairs(tokeniser);

    EXPECT_EQ(keyValuePairs.size(), 1) << "Expected 1 key value pair after parsing";
    EXPECT_EQ(keyValuePairs["inherit"], "atdm:mover_handle_base");
}

TEST(DefTokeniser, StringContinuationWithWhitespace)
{
    std::string testString = R"( "inherit"	"atdm:" \ 	 
    "mover_handle_base")";

    parser::BasicDefTokeniser<std::string> tokeniser(testString);

    auto keyValuePairs = parseStringPairs(tokeniser);

    EXPECT_EQ(keyValuePairs.size(), 1) << "Expected 1 key value pair after parsing";
    EXPECT_EQ(keyValuePairs["inherit"], "atdm:mover_handle_base");
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

    auto keyValuePairs = parseStringPairs(tokeniser);

    EXPECT_EQ(keyValuePairs.size(), 12) << "Expected 12 key value pairs after parsing";

    EXPECT_EQ(keyValuePairs["snd_tap_locked"], "");
    EXPECT_EQ(keyValuePairs["editor_snd snd_tap_locked"], "Called when the handle starts to move and its door is locked.");
    EXPECT_EQ(keyValuePairs["mins"], "-1 -1 -3");
}

}
