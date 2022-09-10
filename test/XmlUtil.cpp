/**
 * Tests covering the XML Document handling classes in namespace xmlutil
 */

#include "RadiantTest.h"
#include "algorithm/FileUtils.h"
#include "string/trim.h"
#include "testutil/TemporaryFile.h"
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
    EXPECT_EQ(document.findXPath("/testDocument/colourscheme/*").size(), 62);
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

}
