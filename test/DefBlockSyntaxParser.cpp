#include "gtest/gtest.h"

#include "parser/DefBlockSyntaxParser.h"

namespace test
{


namespace
{

inline parser::DefSyntaxTree::Ptr parseText(const std::string& text)
{
    parser::DefBlockSyntaxParser<std::string> parser(text);
    return parser.parse();
}

inline void expectToken(const parser::DefSyntaxToken& token, parser::DefSyntaxToken::Type type, const std::string& value)
{
    EXPECT_EQ(token.type, type) << "Expected token type " << (int)type;
    EXPECT_EQ(token.value, value) << "Expected token value " << value;
}

inline void expectSingleToken(const std::string& source, parser::DefSyntaxToken::Type type, const std::string& value)
{
    string::Tokeniser<parser::DefBlockSyntaxTokeniserFunc, std::string::const_iterator, parser::DefSyntaxToken> tokeniser(
        source, parser::DefBlockSyntaxTokeniserFunc()
    );

    auto it = tokeniser.getIterator();
    expectToken(*it++, type, value);
}

}

TEST(DefBlockSyntaxTokeniser, EmptyText)
{
    string::Tokeniser<parser::DefBlockSyntaxTokeniserFunc, std::string::const_iterator, parser::DefSyntaxToken> tokeniser(
        "", parser::DefBlockSyntaxTokeniserFunc()
    );

    auto it = tokeniser.getIterator();
    
    EXPECT_TRUE(it.isExhausted());
}

TEST(DefBlockSyntaxTokeniser, Whitespace)
{
    expectSingleToken(" ", parser::DefSyntaxToken::Type::Whitespace, " ");
    expectSingleToken("\t", parser::DefSyntaxToken::Type::Whitespace, "\t");
    expectSingleToken("\n", parser::DefSyntaxToken::Type::Whitespace, "\n");
    expectSingleToken("\r\n", parser::DefSyntaxToken::Type::Whitespace, "\r\n");
    expectSingleToken("\t\t\r\n", parser::DefSyntaxToken::Type::Whitespace, "\t\t\r\n");
    expectSingleToken("\t \t \r\n  \t\n  \n", parser::DefSyntaxToken::Type::Whitespace, "\t \t \r\n  \t\n  \n");
}

TEST(DefBlockSyntaxTokeniser, SingleTokens)
{
    expectSingleToken("test", parser::DefSyntaxToken::Type::Token, "test");
    expectSingleToken("textures/common", parser::DefSyntaxToken::Type::Token, "textures/common");
    expectSingleToken("m/is/leading*/token/", parser::DefSyntaxToken::Type::Token, "m/is/leading*/token/");
    
    expectSingleToken("// EOL comment ", parser::DefSyntaxToken::Type::EolComment, "// EOL comment ");
    expectSingleToken("//", parser::DefSyntaxToken::Type::EolComment, "//");
    expectSingleToken("//EOLcomment", parser::DefSyntaxToken::Type::EolComment, "//EOLcomment");
    expectSingleToken("/* block comment */", parser::DefSyntaxToken::Type::BlockComment, "/* block comment */");
    expectSingleToken("/* bl/ock * * * comment */", parser::DefSyntaxToken::Type::BlockComment, "/* bl/ock * * * comment */");
    expectSingleToken("/* blk \n test test\n\ncomment */", parser::DefSyntaxToken::Type::BlockComment, "/* blk \n test test\n\ncomment */");
    expectSingleToken("/* this should not crash *", parser::DefSyntaxToken::Type::BlockComment, "/* this should not crash *");
}

void expectTokenSequence(const std::string& source, const std::vector<std::pair<parser::DefSyntaxToken::Type, std::string>>& sequence)
{
    string::Tokeniser<parser::DefBlockSyntaxTokeniserFunc, std::string::const_iterator, parser::DefSyntaxToken> tokeniser(
        source, parser::DefBlockSyntaxTokeniserFunc()
    );

    auto it = tokeniser.getIterator();

    for (const auto& [type, value] : sequence)
    {
        expectToken(*it++, type, value);
    }
}

TEST(DefBlockSyntaxTokeniser, TokenSequences)
{
    expectTokenSequence(" test{}",
    {
        { parser::DefSyntaxToken::Type::Whitespace, " " },
        { parser::DefSyntaxToken::Type::Token, "test" },
        { parser::DefSyntaxToken::Type::BracedBlock, "{}" },
    });

    expectTokenSequence(" test//comment\n{\n{\r\n   TESt \n}\n}",
    {
        { parser::DefSyntaxToken::Type::Whitespace, " " },
        { parser::DefSyntaxToken::Type::Token, "test" },
        { parser::DefSyntaxToken::Type::EolComment, "//comment" },
        { parser::DefSyntaxToken::Type::Whitespace, "\n" },
        { parser::DefSyntaxToken::Type::BracedBlock, "{\n{\r\n   TESt \n}\n}" },
    });

    expectTokenSequence("/*comment*/\ntest/* comment */{{//",
    {
        { parser::DefSyntaxToken::Type::BlockComment, "/*comment*/" },
        { parser::DefSyntaxToken::Type::Whitespace, "\n" },
        { parser::DefSyntaxToken::Type::Token, "test" },
        { parser::DefSyntaxToken::Type::BlockComment, "/* comment */" },
        { parser::DefSyntaxToken::Type::BracedBlock, "{{//" },
    });

    expectTokenSequence("test\n{\n    \"some to//kens {{\" \"containing /* control characters */\" // test\n}\r\n\r\n",
    {
        { parser::DefSyntaxToken::Type::Token, "test" },
        { parser::DefSyntaxToken::Type::Whitespace, "\n" },
        { parser::DefSyntaxToken::Type::BracedBlock, "{\n    \"some to//kens {{\" \"containing /* control characters */\" // test\n}" },
        { parser::DefSyntaxToken::Type::Whitespace, "\r\n\r\n" },
    });
}

TEST(DefBlockSyntaxParser, EmptyText)
{
    auto syntaxTree = parseText("");

    EXPECT_TRUE(syntaxTree) << "Syntax Root must not be null";
}

TEST(DefBlockSyntaxParser, Whitespace)
{
    auto syntaxTree = parseText(" ");
    EXPECT_EQ(syntaxTree->root->getChildren().size(), 1) << "Expected 1 whitespace node";
    EXPECT_EQ(syntaxTree->root->getChildren().front()->getType(), parser::DefSyntaxNode::Type::Whitespace);
    EXPECT_EQ(syntaxTree->root->getChildren().front()->getString(), " ");

    syntaxTree = parseText("\n\n");
    EXPECT_EQ(syntaxTree->root->getChildren().size(), 1) << "Expected 1 whitespace node";
    EXPECT_EQ(syntaxTree->root->getChildren().front()->getType(), parser::DefSyntaxNode::Type::Whitespace);
    EXPECT_EQ(syntaxTree->root->getChildren().front()->getString(), "\n\n");

    syntaxTree = parseText("\t \t");
    EXPECT_EQ(syntaxTree->root->getChildren().size(), 1) << "Expected 1 whitespace node";
    EXPECT_EQ(syntaxTree->root->getChildren().front()->getType(), parser::DefSyntaxNode::Type::Whitespace);
    EXPECT_EQ(syntaxTree->root->getChildren().front()->getString(), "\t \t");

    syntaxTree = parseText("\r\n \r\n");
    EXPECT_EQ(syntaxTree->root->getChildren().size(), 1) << "Expected 1 whitespace node";
    EXPECT_EQ(syntaxTree->root->getChildren().front()->getType(), parser::DefSyntaxNode::Type::Whitespace);
    EXPECT_EQ(syntaxTree->root->getChildren().front()->getString(), "\r\n \r\n");
}

}
