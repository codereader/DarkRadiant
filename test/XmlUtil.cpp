/**
 * Tests covering the XML Document handling classes in namespace xmlutil
 */

#include "RadiantTest.h"
#include "algorithm/FileUtils.h"
#include "string/trim.h"
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

TEST_F(XmlTest, CreateDocumentCopy)
{
    // Copy-construct an XML document, prove that the copies are not linked
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
    // Insert a document/node tree into an existing document
}

TEST_F(XmlTest, CopyNodesIntoDocument)
{
    // Copy existing nodes into this document
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
}

TEST_F(XmlTest, SaveDocumentToFile)
{
    // Test the saveToFile() method
}

TEST_F(XmlTest, SaveDocumentToString)
{
    // Test the saveToString() method
    auto filename = _context.getTestResourcePath() + TEST_XML_FILE;

    xml::Document document(filename);

    auto fileContents = algorithm::loadFileToString(filename);

    // saveToString() should produce the same file again
    EXPECT_EQ(document.saveToString(), fileContents);
}

}
