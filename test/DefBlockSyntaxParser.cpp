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
