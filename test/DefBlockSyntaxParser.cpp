#include "RadiantTest.h"

#include "parser/DefBlockSyntaxParser.h"
#include "algorithm/FileUtils.h"

namespace test
{


namespace
{

inline parser::DefSyntaxTree::Ptr parseText(const std::string& text)
{
    parser::DefBlockSyntaxParser<const std::string> parser(text);
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

    expectTokenSequence("test\n{\n\"some\" \"keyvalue\" // line comment\n\n}\r\n\r\ntest2\n{\n\"some\" \"keyvalue\"\n\n}\n",
    {
        { parser::DefSyntaxToken::Type::Token, "test" },
        { parser::DefSyntaxToken::Type::Whitespace, "\n" },
        { parser::DefSyntaxToken::Type::BracedBlock, "{\n\"some\" \"keyvalue\" // line comment\n\n}" },
        { parser::DefSyntaxToken::Type::Whitespace, "\r\n\r\n" },
        { parser::DefSyntaxToken::Type::Token, "test2" },
        { parser::DefSyntaxToken::Type::Whitespace, "\n" },
        { parser::DefSyntaxToken::Type::BracedBlock, "{\n\"some\" \"keyvalue\"\n\n}" },
        { parser::DefSyntaxToken::Type::Whitespace, "\n" },
    });

    expectTokenSequence(R"(// Test declarations used for some DeclarationManager unit tests

decl/exporttest/guisurf1
{
    guiSurf	guis/lvlmaps/genericmap.gui
}

testdecl2 decltable2 { { 0, 0, 0, 0, 1, 1 } }
testdecl2 decltable3 { { 0, 0, 0, 0, 1, 1 } }
)",
    {
        { parser::DefSyntaxToken::Type::EolComment, "// Test declarations used for some DeclarationManager unit tests" },
        { parser::DefSyntaxToken::Type::Whitespace, "\n\n" },
        { parser::DefSyntaxToken::Type::Token, "decl/exporttest/guisurf1" },
        { parser::DefSyntaxToken::Type::Whitespace, "\n" },
        { parser::DefSyntaxToken::Type::BracedBlock, R"({
    guiSurf	guis/lvlmaps/genericmap.gui
})" },
        { parser::DefSyntaxToken::Type::Whitespace, "\n\n" },
        { parser::DefSyntaxToken::Type::Token, "testdecl2" },
        { parser::DefSyntaxToken::Type::Whitespace, " " },
        { parser::DefSyntaxToken::Type::Token, "decltable2" },
        { parser::DefSyntaxToken::Type::Whitespace, " " },
        { parser::DefSyntaxToken::Type::BracedBlock, "{ { 0, 0, 0, 0, 1, 1 } }" },
        { parser::DefSyntaxToken::Type::Whitespace, "\n" },
        { parser::DefSyntaxToken::Type::Token, "testdecl2" },
        { parser::DefSyntaxToken::Type::Whitespace, " " },
        { parser::DefSyntaxToken::Type::Token, "decltable3" },
        { parser::DefSyntaxToken::Type::Whitespace, " " },
        { parser::DefSyntaxToken::Type::BracedBlock, "{ { 0, 0, 0, 0, 1, 1 } }" },
        { parser::DefSyntaxToken::Type::Whitespace, "\n" },
    });
}

using DefBlockSyntaxParserTest = RadiantTest;

TEST_F(DefBlockSyntaxParserTest, EmptyText)
{
    auto syntaxTree = parseText("");
    EXPECT_TRUE(syntaxTree) << "Syntax Root must not be null";
}

TEST_F(DefBlockSyntaxParserTest, Whitespace)
{
    auto syntaxTree = parseText(" ");
    EXPECT_EQ(syntaxTree->getRoot()->getChildren().size(), 1) << "Expected 1 whitespace node";
    EXPECT_EQ(syntaxTree->getRoot()->getChildren().front()->getType(), parser::DefSyntaxNode::Type::Whitespace);
    EXPECT_EQ(syntaxTree->getRoot()->getChildren().front()->getString(), " ");

    syntaxTree = parseText("\n\n");
    EXPECT_EQ(syntaxTree->getRoot()->getChildren().size(), 1) << "Expected 1 whitespace node";
    EXPECT_EQ(syntaxTree->getRoot()->getChildren().front()->getType(), parser::DefSyntaxNode::Type::Whitespace);
    EXPECT_EQ(syntaxTree->getRoot()->getChildren().front()->getString(), "\n\n");

    syntaxTree = parseText("\t \t");
    EXPECT_EQ(syntaxTree->getRoot()->getChildren().size(), 1) << "Expected 1 whitespace node";
    EXPECT_EQ(syntaxTree->getRoot()->getChildren().front()->getType(), parser::DefSyntaxNode::Type::Whitespace);
    EXPECT_EQ(syntaxTree->getRoot()->getChildren().front()->getString(), "\t \t");

    syntaxTree = parseText("\r\n \r\n");
    EXPECT_EQ(syntaxTree->getRoot()->getChildren().size(), 1) << "Expected 1 whitespace node";
    EXPECT_EQ(syntaxTree->getRoot()->getChildren().front()->getType(), parser::DefSyntaxNode::Type::Whitespace);
    EXPECT_EQ(syntaxTree->getRoot()->getChildren().front()->getString(), "\r\n \r\n");
}

namespace
{

    const std::string ExampleFileContent = R"(// Some comment

decl/exporttest/guisurf1
{
    guiSurf	guis/lvlmaps/genericmap.gui
}

testdecl2 decltable2 { { 0, 0, 0, 0, 1, 1 } }
testdecl2 decltable3 { { 0, 0, 0, 0, 1, 1 } }
)";

    void expectExampleFileSyntax(const parser::DefSyntaxTree::Ptr& syntaxTree)
    {
        EXPECT_EQ(syntaxTree->getRoot()->getChildren().size(), 8) << "Expected 8 nodes";

        EXPECT_EQ(syntaxTree->getRoot()->getChildren().at(0)->getType(), parser::DefSyntaxNode::Type::Comment);
        EXPECT_EQ(syntaxTree->getRoot()->getChildren().at(0)->getString(), "// Some comment");

        EXPECT_EQ(syntaxTree->getRoot()->getChildren().at(1)->getType(), parser::DefSyntaxNode::Type::Whitespace);
        EXPECT_EQ(syntaxTree->getRoot()->getChildren().at(1)->getString(), "\n\n");

        EXPECT_EQ(syntaxTree->getRoot()->getChildren().at(2)->getType(), parser::DefSyntaxNode::Type::DeclBlock);
        auto block = std::static_pointer_cast<parser::DefBlockSyntax>(syntaxTree->getRoot()->getChildren().at(2));
        EXPECT_FALSE(block->getType());
        EXPECT_EQ(block->getName()->getString(), "decl/exporttest/guisurf1");
        EXPECT_EQ(block->getBlockContents(), "\n    guiSurf	guis/lvlmaps/genericmap.gui\n");

        EXPECT_EQ(syntaxTree->getRoot()->getChildren().at(3)->getType(), parser::DefSyntaxNode::Type::Whitespace);
        EXPECT_EQ(syntaxTree->getRoot()->getChildren().at(3)->getString(), "\n\n");

        EXPECT_EQ(syntaxTree->getRoot()->getChildren().at(4)->getType(), parser::DefSyntaxNode::Type::DeclBlock);
        block = std::static_pointer_cast<parser::DefBlockSyntax>(syntaxTree->getRoot()->getChildren().at(4));
        EXPECT_EQ(block->getType()->getString(), "testdecl2");
        EXPECT_EQ(block->getName()->getString(), "decltable2");
        EXPECT_EQ(block->getBlockContents(), " { 0, 0, 0, 0, 1, 1 } ");

        EXPECT_EQ(syntaxTree->getRoot()->getChildren().at(5)->getType(), parser::DefSyntaxNode::Type::Whitespace);
        EXPECT_EQ(syntaxTree->getRoot()->getChildren().at(5)->getString(), "\n");

        EXPECT_EQ(syntaxTree->getRoot()->getChildren().at(6)->getType(), parser::DefSyntaxNode::Type::DeclBlock);
        block = std::static_pointer_cast<parser::DefBlockSyntax>(syntaxTree->getRoot()->getChildren().at(6));
        EXPECT_EQ(block->getType()->getString(), "testdecl2");
        EXPECT_EQ(block->getName()->getString(), "decltable3");
        EXPECT_EQ(block->getBlockContents(), " { 0, 0, 0, 0, 1, 1 } ");

        EXPECT_EQ(syntaxTree->getRoot()->getChildren().at(7)->getType(), parser::DefSyntaxNode::Type::Whitespace);
        EXPECT_EQ(syntaxTree->getRoot()->getChildren().at(7)->getString(), "\n");
    }
}

// Parse the example decl file contents from a string and check the syntax tree contents
TEST_F(DefBlockSyntaxParserTest, SimpleDeclFileFromString)
{
    parser::DefBlockSyntaxParser<const std::string> parser(ExampleFileContent);
    auto tree = parser.parse();
    
    expectExampleFileSyntax(tree);
}

// Parse the example decl file contents from a stream and check the syntax tree contents
TEST_F(DefBlockSyntaxParserTest, SimpleDeclFileFromStream)
{
    std::stringstream stream(ExampleFileContent);

    parser::DefBlockSyntaxParser<std::istream> parser(stream);
    auto tree = parser.parse();
    
    expectExampleFileSyntax(tree);
}

void checkDeclFileReconstruction(const std::string& declFile)
{
    auto originalText = algorithm::loadTextFromVfsFile(declFile);
    auto syntaxTree = parseText(originalText);

    auto reconstructedText = syntaxTree->getString();
    EXPECT_EQ(reconstructedText, originalText) << "Parsed file " << declFile << " couldn't be reconstructed";
}
// Attempt to parse a whole file and reconstruct it from the syntax tree
TEST_F(DefBlockSyntaxParserTest, ReconstructFileFromSyntaxTree)
{
    checkDeclFileReconstruction("testdecls/exporttest.decl");
    checkDeclFileReconstruction("testdecls/numbers.decl");
    checkDeclFileReconstruction("testdecls/precedence_test1.decl");
    checkDeclFileReconstruction("testdecls/precedence_test2.decl");
    checkDeclFileReconstruction("testdecls/removal_tests.decl");
    checkDeclFileReconstruction("testdecls/removal_tests.decl");

    checkDeclFileReconstruction("particles/testparticles.prt");
    
    checkDeclFileReconstruction("materials/parsertest.mtr");
    checkDeclFileReconstruction("materials/example.mtr");
    checkDeclFileReconstruction("materials/tdm_internal_engine.mtr");

    checkDeclFileReconstruction("def/base.def");
    checkDeclFileReconstruction("def/tdm_ai.def");
    checkDeclFileReconstruction("def/mover_door.def");
}

}
