/**
 * Tests covering the XML Document handling classes in namespace xmlutil
 */

#include "RadiantTest.h"
#include "algorithm/FileUtils.h"
#include "string/trim.h"
#include "testutil/TemporaryFile.h"
#include "xmlutil/Node.h"
#include "xmlutil/Document.h"

namespace test
{

using XmlTest = RadiantTest;

// Relative to the resource folder
constexpr const char* const TEST_XML_FILE = "xml/testfile.xml";

TEST_F(XmlTest, CreateEmptyDocument)
{
    // Create an xmlutil::Document from scratch
    auto document = xml::Document::create();

    EXPECT_TRUE(document.isValid()) << "New document must be valid";
    EXPECT_EQ(document.getTopLevelNode().getName(), "") << "New document doesn't have a top level node";
    EXPECT_EQ(string::trim_copy(document.saveToString()), "<?xml version=\"1.0\" encoding=\"utf-8\"?>");
}

TEST_F(XmlTest, CreateDocumentFromFile)
{
    // Create an xmlutil::Document from an existing XML file
    xml::Document document(_context.getTestResourcePath() + TEST_XML_FILE);

    EXPECT_TRUE(document.isValid()) << "Test file could not be loaded";
    EXPECT_EQ(document.getTopLevelNode().getName(), "testDocument");
}

TEST_F(XmlTest, CreateDocumentFromStream)
{
    // Create an xmlutil::Document from an input stream
    std::ifstream stream(_context.getTestResourcePath() + TEST_XML_FILE);

    xml::Document document(stream);

    EXPECT_TRUE(document.isValid()) << "Test stream could not be parsed";
    EXPECT_EQ(document.getTopLevelNode().getName(), "testDocument");
}

TEST_F(XmlTest, AddTopLevelNodeToDocument)
{
    // Create an empty document and add a top level node to it
    auto document = xml::Document::create();
    EXPECT_EQ(document.getTopLevelNode().getName(), "");

    document.addTopLevelNode("someNodeName");
    EXPECT_EQ(document.getTopLevelNode().getName(), "someNodeName");

    // Calling addTopLevelNode removes the previous top level node
    document.addTopLevelNode("replacementNodeName");
    EXPECT_EQ(document.getTopLevelNode().getName(), "replacementNodeName");
}

TEST_F(XmlTest, ImportDocument)
{
    // Create an empty document, add an <importNode /> tag without children
    auto importDocument = xml::Document::create();
    importDocument.addTopLevelNode("importNode");

    // Load an existing file
    xml::Document document(_context.getTestResourcePath() + TEST_XML_FILE);

    // Import the <importNode /> into this document
    auto targetNode = document.findXPath("//colourscheme[@name='Black & Green']").at(0);
    auto previousChildCount = document.findXPath("//colourscheme[@name='Black & Green']/*").size();

    document.importDocument(importDocument, targetNode);

    auto newChildCount = document.findXPath("//colourscheme[@name='Black & Green']/*").size();
    EXPECT_EQ(newChildCount, previousChildCount + 1);

    // Importing a node to a target node without existing children should work too
    targetNode = document.findXPath("//colourscheme[@name='Black & Green']/colour[@name='active_view_name']").at(0);

    // The target node shouldn't have any children yet
    EXPECT_EQ(document.findXPath("//colourscheme[@name='Black & Green']/colour[@name='active_view_name']/*").size(), 0);

    auto importDocument2 = xml::Document::create();
    importDocument2.addTopLevelNode("anotherNode");

    document.importDocument(importDocument2, targetNode);

    // We should have one child node now
    EXPECT_EQ(document.findXPath("//colourscheme[@name='Black & Green']/colour[@name='active_view_name']/*").size(), 1);
}

TEST_F(XmlTest, CopyNodesIntoDocument)
{
    // Copy existing nodes into this document
    auto targetDocument = xml::Document::create();
    targetDocument.addTopLevelNode("importNode");

    // Load an existing file
    xml::Document document(_context.getTestResourcePath() + TEST_XML_FILE);

    // Copy all colourScheme nodes
    auto importNodes = document.findXPath("//colourscheme");
    EXPECT_EQ(importNodes.size(), 2) << "Expect 2 colour schemes to be present";

    EXPECT_EQ(targetDocument.findXPath("//colourscheme").size(), 0) << "Target document shouldn't have any colourscheme nodes yet";

    targetDocument.copyNodes(importNodes);

    // The 2 colour schemes should have been added below the top level node
    EXPECT_EQ(targetDocument.findXPath("/importNode/colourscheme").size(), 2) << "Target document should have 2 colourscheme nodes now";
}

TEST_F(XmlTest, DocumentValidityCheck)
{
    // Test the isValid() method
    // Loading an invalid file will produce an invalid document
    auto filename = _context.getTestResourcePath() + "xml/broken_file.xml";

    xml::Document broken(filename);
    EXPECT_FALSE(broken.isValid());

    // Same goes for the stream constructor
    std::ifstream stream(filename);
    xml::Document brokenFromStream(stream);
    EXPECT_FALSE(brokenFromStream.isValid());
}

TEST_F(XmlTest, FindXPathInDocument)
{
    // Test the findXPath() method
    xml::Document document(_context.getTestResourcePath() + TEST_XML_FILE);

    // Find the root element
    EXPECT_EQ(document.findXPath("/testDocument").size(), 1);

    // Find the root node everywhere
    EXPECT_EQ(document.findXPath("//testDocument").size(), 1);

    // Find the 2 colourScheme nodes
    EXPECT_EQ(document.findXPath("/testDocument/colourscheme").size(), 2);

    // Find the 2 colourScheme nodes with the double-slash search
    EXPECT_EQ(document.findXPath("//colourscheme").size(), 2);

    // Find the 2 colourScheme nodes using a wildcard expression
    EXPECT_EQ(document.findXPath("/testDocument/*").size(), 2);

    // XPath expressions are case sensitive
    EXPECT_EQ(document.findXPath("/testDocument/colourSCHEME").size(), 0); // find nothing

    // Find all colour nodes using various methods
    EXPECT_EQ(document.findXPath("/testDocument/colourscheme/colour").size(), 62);
    EXPECT_EQ(document.findXPath("/testDocument/colourscheme/*").size(), 64);
    EXPECT_EQ(document.findXPath("/testDocument//colour").size(), 62);
    EXPECT_EQ(document.findXPath("//colour").size(), 62);

    // Find specific colour scheme(s)
    EXPECT_EQ(document.findXPath("//colourscheme[@name='Black & Green']").size(), 1);
    EXPECT_EQ(document.findXPath("//colourscheme[@version='1.0']").size(), 2);
    EXPECT_EQ(document.findXPath("/testDocument/colourscheme[@name='Black & Green']").size(), 1);
    EXPECT_EQ(document.findXPath("/testDocument/*[@name='Black & Green']").size(), 1);

    // Ampersand doesn't need to be escaped, otherwise there's no result
    EXPECT_EQ(document.findXPath("//colourscheme[@name='Black &amp; Green']").size(), 0);
}

TEST_F(XmlTest, SaveDocumentToFile)
{
    // Test the saveToFile() method
    auto filename = _context.getTestResourcePath() + TEST_XML_FILE;
    auto targetFilePath = _context.getTestResourcePath() + TEST_XML_FILE + ".tmp";
    TemporaryFile targetFile(targetFilePath);

    xml::Document document(filename);
    auto fileContents = algorithm::loadFileToString(filename);

    // saveToFile() should produce the same file again, except for trailing line breaks
    document.saveToFile(targetFilePath);
    auto newFileContents = algorithm::loadFileToString(targetFilePath);

    EXPECT_EQ(string::trim_copy(newFileContents), fileContents);
}

TEST_F(XmlTest, SaveDocumentToString)
{
    // Test the saveToString() method
    auto filename = _context.getTestResourcePath() + TEST_XML_FILE;

    xml::Document document(filename);

    auto fileContents = algorithm::loadFileToString(filename);

    // saveToString() should produce the same file again, except for trailing line breaks
    EXPECT_EQ(string::trim_copy(document.saveToString()), fileContents);
}

TEST_F(XmlTest, CreateEmptyNode)
{
    // Test the saveToString() method
    xml::Node node(nullptr, nullptr);

    EXPECT_FALSE(node.isValid()) << "Empty node should not be valid";
}

TEST_F(XmlTest, NodeName)
{
    xml::Document document(_context.getTestResourcePath() + TEST_XML_FILE);

    // Get the root node and check its name
    auto rootNode = document.findXPath("/testDocument").at(0);

    EXPECT_TRUE(rootNode.isValid()) << "Root node should be valid";
    EXPECT_EQ(rootNode.getName(), "testDocument") << "Root node has the wrong name";
}

TEST_F(XmlTest, CopyConstructNode)
{
    xml::Document document(_context.getTestResourcePath() + TEST_XML_FILE);

    // Get the root node and check its name
    auto rootNode = document.findXPath("/testDocument").at(0);

    EXPECT_TRUE(rootNode.isValid()) << "Root node should be valid";
    EXPECT_EQ(rootNode.getName(), "testDocument") << "Root node has the wrong name";

    xml::Node copy(rootNode);
    EXPECT_TRUE(copy.isValid()) << "Root node should be valid";
    EXPECT_EQ(copy.getName(), "testDocument") << "Copied node has the wrong name";

    // Copying an empty node should be possible
    xml::Node nullNode(nullptr, nullptr);
    xml::Node copiedNullNode(nullNode);

    EXPECT_FALSE(nullNode.isValid()) << "Empty node should not be valid";
    EXPECT_FALSE(copiedNullNode.isValid()) << "Empty copied node should not be valid";

    // operator=
    xml::Node assignedNullNode(rootNode);
    assignedNullNode = nullNode;
    EXPECT_FALSE(assignedNullNode.isValid()) << "Empty assigned node should not be valid";
}

TEST_F(XmlTest, GetNodeChildren)
{
    xml::Document document(_context.getTestResourcePath() + TEST_XML_FILE);

    // Get the root node and check its name
    auto rootNode = document.findXPath("/testDocument").at(0);

    EXPECT_TRUE(rootNode.isValid()) << "Root node should be valid";

    auto children = rootNode.getChildren();

    EXPECT_GE(children.size(), 2) << "Expected at least 2 child nodes";

    // Check all non-text nodes
    for (const auto& child : children)
    {
        if (child.getName() == "text")
        {
            continue;
        }

        EXPECT_EQ(child.getName(), "colourscheme") << "Expected only colourscheme nodes";
    }
}

TEST_F(XmlTest, GetNamedNodeChildren)
{
    xml::Document document(_context.getTestResourcePath() + TEST_XML_FILE);

    // Get the root node and check its name
    auto rootNode = document.findXPath("/testDocument").at(0);

    EXPECT_TRUE(rootNode.isValid()) << "Root node should be valid";

    auto children = rootNode.getNamedChildren("colourscheme");

    EXPECT_EQ(children.size(), 2) << "Expected exactly 2 child nodes";

    EXPECT_EQ(children.at(0).getName(), "colourscheme") << "Expected only colourscheme nodes";
    EXPECT_EQ(children.at(1).getName(), "colourscheme") << "Expected only colourscheme nodes";
}

TEST_F(XmlTest, GetNodeAttributeValue)
{
    xml::Document document(_context.getTestResourcePath() + TEST_XML_FILE);

    // Get the root node and check its name
    auto colourNode = document.findXPath("/testDocument//colour[@name='active_view_name']").at(0);

    EXPECT_TRUE(colourNode.isValid()) << "Colour node should be valid";
    EXPECT_EQ(colourNode.getName(), "colour") << "Expected only colourscheme nodes";

    EXPECT_EQ(colourNode.getAttributeValue("name"), "active_view_name") << "Attribute values not retrieved";
    EXPECT_EQ(colourNode.getAttributeValue("value"), "0.7 0.7 0") << "Attribute values not retrieved";
    EXPECT_EQ(colourNode.getAttributeValue("nonexistent"), "") << "Nonexistent attributes should return an empty string";
}

TEST_F(XmlTest, SetNodeAttributeValue)
{
    xml::Document document(_context.getTestResourcePath() + TEST_XML_FILE);

    // Get the root node and check its name
    auto colourNode = document.findXPath("/testDocument//colour[@name='active_view_name']").at(0);

    EXPECT_EQ(colourNode.getAttributeValue("name"), "active_view_name") << "Attribute values not retrieved";
    EXPECT_EQ(colourNode.getAttributeValue("nonexistent"), "") << "Nonexistent attributes should return an empty string";
    EXPECT_EQ(document.findXPath("/testDocument//colour[@name='active_view_name']").size(), 2) << "We should start with 2 active_view_name colours";
    EXPECT_EQ(document.findXPath("/testDocument//colour[@name='My new name!']").size(), 0) << "This node should not be there yet";

    colourNode.setAttributeValue("name", "My new name!");
    colourNode.setAttributeValue("nonexistent", "Not so nonexistent anymore");

    EXPECT_EQ(colourNode.getAttributeValue("name"), "My new name!") << "Attribute values not retrieved";
    EXPECT_EQ(colourNode.getAttributeValue("nonexistent"), "Not so nonexistent anymore") << "Attribute values not retrieved";

    // The changes should also be applied within the document without explicitly having to save anything
    EXPECT_EQ(document.findXPath("/testDocument//colour[@name='active_view_name']").size(), 1) << "Only one of these should be left";
    EXPECT_EQ(document.findXPath("/testDocument//colour[@name='My new name!']").size(), 1);
}

TEST_F(XmlTest, CreateNodeChild)
{
    xml::Document document(_context.getTestResourcePath() + TEST_XML_FILE);

    EXPECT_EQ(document.findXPath("//colourscheme[@name='Black & Green']").size(), 1);
    EXPECT_EQ(document.findXPath("//colourscheme[@name='Black & Green']/testNode").size(), 0) << "Expected no children";

    auto colourscheme = document.findXPath("//colourscheme[@name='Black & Green']").at(0);
    auto previousChildNodes = colourscheme.getChildren();

    colourscheme.createChild("testNode");

    EXPECT_EQ(document.findXPath("//colourscheme[@name='Black & Green']").size(), 1);
    EXPECT_EQ(document.findXPath("//colourscheme[@name='Black & Green']/testNode").size(), 1) << "Expected 1 child now";

    auto childNodes = colourscheme.getChildren();
    EXPECT_EQ(childNodes.size(), previousChildNodes.size() + 1);

    childNodes = colourscheme.getNamedChildren("testNode");
    EXPECT_EQ(childNodes.size(), 1);
    EXPECT_EQ(childNodes.at(0).getName(), "testNode");
}

TEST_F(XmlTest, GetNodeContent)
{
    xml::Document document(_context.getTestResourcePath() + TEST_XML_FILE);

    auto colourNode = document.findXPath("//colourscheme[@name='DarkRadiant Default']/colour[@name='xyview_crosshairs']").at(0);
    EXPECT_EQ(colourNode.getContent(), "") << "Closed XML tag should have an empty content";

    EXPECT_EQ(document.findXPath("//colourscheme[@name='DarkRadiant Default']").size(), 1);
    EXPECT_EQ(document.findXPath("//colourscheme[@name='DarkRadiant Default']/specialNode").size(), 1) << "Expected 1 matching node";

    auto specialNode = document.findXPath("//colourscheme[@name='DarkRadiant Default']/specialNode").at(0);
    EXPECT_EQ(specialNode.getContent(), "Some special content");

    auto specialNodeWithWhitespace = document.findXPath("//colourscheme[@name='DarkRadiant Default']/specialNodeWithWhitespace").at(0);
    EXPECT_EQ(specialNodeWithWhitespace.getContent(), R"(
        Some special content including leading and trailing whitespace
    )");
}

TEST_F(XmlTest, SetNodeContent)
{
    xml::Document document(_context.getTestResourcePath() + TEST_XML_FILE);

    EXPECT_EQ(document.findXPath("//colourscheme[@name='DarkRadiant Default']").size(), 1);
    EXPECT_EQ(document.findXPath("//colourscheme[@name='DarkRadiant Default']/specialNode").size(), 1) << "Expected 1 matching node";

    auto specialNode = document.findXPath("//colourscheme[@name='DarkRadiant Default']/specialNode").at(0);

    std::string someText = "-Some text-";
    specialNode.setContent(someText);
    EXPECT_EQ(specialNode.getContent(), someText);
    EXPECT_NE(document.saveToString().find(someText), std::string::npos) << "Expected to find the custom string in the document";

    // LF whitespaces should be preserved when stored in the content
    std::string someTextWithWhitespace = "\n-Some\ntext-\t\t\n";
    specialNode.setContent(someTextWithWhitespace);
    EXPECT_EQ(specialNode.getContent(), someTextWithWhitespace);
    EXPECT_NE(document.saveToString().find(someTextWithWhitespace), std::string::npos) << "Expected to find the custom whitespace string in the document";
}

TEST_F(XmlTest, AddTextToNode)
{
    xml::Document document(_context.getTestResourcePath() + TEST_XML_FILE);

    auto colourscheme = document.findXPath("//colourscheme[@name='DarkRadiant Default']").at(0);
    auto testNode1 = colourscheme.createChild("testNode");
    auto testNode2 = colourscheme.createChild("testNode");

    // The two nodes lack any whitespace
    auto expectedText = "<testNode/><testNode/>";
    auto expectedTextWithAddedText = "<testNode/> \t  <testNode/>\n";
    EXPECT_NE(document.saveToString().find(expectedText), std::string::npos) << "Expected to find the new nodes in the document";
    EXPECT_EQ(document.saveToString().find(expectedTextWithAddedText), std::string::npos) << "Whitespaced text should not be present yet";

    // Add some specific whitespace to the first node and a line break to the second
    testNode1.addText(" \t  ");
    testNode2.addText("\n");

    EXPECT_EQ(document.saveToString().find(expectedText), std::string::npos) << "The old text should be gone";
    EXPECT_NE(document.saveToString().find(expectedTextWithAddedText), std::string::npos) << "Expected to find the whitespaced nodes in the document";
}

TEST_F(XmlTest, EraseNode)
{
    xml::Document document(_context.getTestResourcePath() + TEST_XML_FILE);

    // Expect exactly two colour schemes, one with that name
    EXPECT_EQ(document.findXPath("//colourscheme").size(), 2);
    EXPECT_EQ(document.findXPath("//colourscheme[@name='DarkRadiant Default']").size(), 1);

    auto colourscheme = document.findXPath("//colourscheme[@name='DarkRadiant Default']").at(0);
    colourscheme.erase();

    EXPECT_EQ(document.findXPath("//colourscheme").size(), 1);
    EXPECT_EQ(document.findXPath("//colourscheme[@name='DarkRadiant Default']").size(), 0);
}

}
