#include "RadiantTest.h"

#include "parser/DefBlockSyntaxParser.h"
#include "algorithm/FileUtils.h"
#include "string/split.h"

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

inline parser::DefBlockSyntaxParser<const std::string>::Tokeniser createTokeniser(const std::string& source)
{
    return parser::DefBlockSyntaxParser<const std::string>::Tokeniser(
        parser::detail::SyntaxParserTraits<const std::string>::GetStartIterator(source),
        parser::detail::SyntaxParserTraits<const std::string>::GetEndIterator(source), 
        parser::DefBlockSyntaxTokeniserFunc()
    );
}

inline void expectSingleToken(const std::string& source, parser::DefSyntaxToken::Type type, const std::string& value)
{
    auto tokeniser = createTokeniser(source);

    auto it = tokeniser.getIterator();
    expectToken(*it++, type, value);
}

}

TEST(DefBlockSyntaxTokeniser, EmptyText)
{
    auto tokeniser = createTokeniser("");
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
    auto tokeniser = createTokeniser(source);
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

TEST_F(DefBlockSyntaxParserTest, ParseEmptyText)
{
    auto syntaxTree = parseText("");
    EXPECT_TRUE(syntaxTree) << "Syntax Root must not be null";
}

TEST_F(DefBlockSyntaxParserTest, ParseWhitespace)
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

inline void parseBlock(const std::string& testString,
    const std::vector<std::pair<std::string, std::string>>& expectedBlocks)
{
    // Use the stream parser variant (even though we could parse right from the string)
    std::istringstream stream{ testString };
    parser::DefBlockSyntaxParser<std::istream> parser(stream);
    auto syntaxTree = parser.parse();

    // Check that the expected blocks appear in the syntax tree, in the correct order
    auto expectedBlock = expectedBlocks.begin();

    syntaxTree->foreachBlock([&](const parser::DefBlockSyntax::Ptr& block)
    {
        if (expectedBlock->first.find(' ') != std::string::npos)
        {
            // Check name and type
            std::vector<std::string> parts;
            string::split(parts, expectedBlock->first, " \t");
            EXPECT_EQ(block->getType()->getString(), parts[0]);
            EXPECT_EQ(block->getName()->getString(), parts[1]);
        }
        else
        {
            if (expectedBlock->first.empty())
            {
                EXPECT_TRUE(!block->getName() || block->getName()->getString().empty());
            }
            else
            {
                EXPECT_EQ(block->getName()->getString(), expectedBlock->first);
            }
        }

        EXPECT_NE(block->getBlockContents().find(expectedBlock->second), std::string::npos);

        ++expectedBlock;
    });
}

inline void parseBlock(const std::string& testString,
    const std::string& expectedName, const std::string& needleToFindInBlockContents)
{
    parseBlock(testString, { std::make_pair(expectedName, needleToFindInBlockContents) });
}

TEST_F(DefBlockSyntaxParserTest, ParseTypedBlock)
{
    std::string testString = R"(entityDef atdm:target_base
{
	"inherit"				"atdm:entity_base"

	"editor_displayFolder"	"Targets"
	"nodraw"				"1"
})";

    parseBlock(testString, "entityDef atdm:target_base", "\"atdm:entity_base\"");
}

TEST_F(DefBlockSyntaxParserTest, ParseTypedBlockWithWhitespace)
{
    std::string testString = R"(	entityDef	atdm:target_base  		
{
	"inherit"				"atdm:entity_base"

	"editor_displayFolder"	"Inner opening { and closing } braces to confuse the parser"
	"nodraw"				"1"
})";

    parseBlock(testString, "entityDef atdm:target_base", "\"atdm:entity_base\"");
}

TEST_F(DefBlockSyntaxParserTest, ParseTypedBlockWithCurlyBracesInContent)
{
    std::string testString = R"(	entityDef	atdm:target_base  		
{
	"editor_displayFolder"	"Inner opening { and closing } braces to confuse the parser"
	"nodraw"				"1"
})";

    parseBlock(testString, "entityDef atdm:target_base", "Inner opening { and closing } braces to confuse the parser");
}

TEST_F(DefBlockSyntaxParserTest, ParseNestedBlock)
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

TEST_F(DefBlockSyntaxParserTest, ParseNameAndBlockInSeparateLines)
{
    std::string testString = R"(textures/parsing_test/variant1
    {
        diffusemap _white
    })";

    parseBlock(testString, "textures/parsing_test/variant1", "_white");
}

TEST_F(DefBlockSyntaxParserTest, ParseCommentAfterBlockName)
{
    std::string testString = R"(textures/parsing_test/commentAfterBlockName // comment separated by whitespace
    {
        diffusemap _white
    })";

    parseBlock(testString, "textures/parsing_test/commentAfterBlockName", "_white");
}

TEST_F(DefBlockSyntaxParserTest, ParseCommentAfterBlockNameNoWhitespace)
{
    std::string testString = R"(textures/parsing_test/CommentAfterBlockNameNoWhitespace// comment without whitespace
    {
        diffusemap _white
    })";

    parseBlock(testString, "textures/parsing_test/CommentAfterBlockNameNoWhitespace", "_white");
}

TEST_F(DefBlockSyntaxParserTest, ParseCommentInBetweenNameAndBlock)
{
    std::string testString = R"(textures/parsing_test/commentInBetweenNameAndBlock
// comment between name and block
{
    diffusemap _white
})";

    parseBlock(testString, "textures/parsing_test/commentInBetweenNameAndBlock", "_white");
}

TEST_F(DefBlockSyntaxParserTest, ParseNameAndBlockInSameLine)
{
    std::string testString = R"(textures/parsing_test/variant2 {
        diffusemap _white
    })";

    parseBlock(testString, "textures/parsing_test/variant2", "_white");
}

TEST_F(DefBlockSyntaxParserTest, ParseNameAndBlockInSameLineNoWhitespace)
{
    std::string testString = R"(textures/parsing_test/variant3{
        diffusemap _white
    })";

    parseBlock(testString, "textures/parsing_test/variant3", "_white");
}

TEST_F(DefBlockSyntaxParserTest, ParseTightBlockSequence)
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
TEST_F(DefBlockSyntaxParserTest, ParseWhitespaceAfterTypename)
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

TEST_F(DefBlockSyntaxParserTest, ParseUnnamedBlock)
{
    std::string testString = R"(/* just a comment */ {
        diffusemap _white
    })";

    parseBlock(testString, "", "_white");
}

TEST_F(DefBlockSyntaxParserTest, ParseBlockWithTooManyLeadingTokens)
{
    std::string testString = R"(testdecl blabla something {
        diffusemap _white
    })";

    parseBlock(testString, "testdecl blabla", "_white");
}

TEST_F(DefBlockSyntaxParserTest, ParseIncompleteBlock)
{
    std::string testString = R"(testdecl something {
        diffusemap _white)";

    parseBlock(testString, "something", "_white");
}

}
