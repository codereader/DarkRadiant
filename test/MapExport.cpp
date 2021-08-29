#include "RadiantTest.h"

#include "imap.h"
#include "imapformat.h"
#include "ibrush.h"
#include "iselection.h"
#include "scenelib.h"
#include "os/path.h"
#include "string/predicate.h"
#include "xmlutil/Document.h"
#include "messages/MapFileOperation.h"
#include "algorithm/XmlUtils.h"
#include "algorithm/Primitives.h"
#include "testutil/FileSelectionHelper.h"

namespace test
{

namespace
{

void createAndSelectSingleBrush()
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();

    auto brushNode = GlobalBrushCreator().createBrush();
    worldspawn->addChildNode(brushNode);

    GlobalSelectionSystem().setSelectedAll(false);
    Node_setSelected(brushNode, true);
}

}

using MapExportTest = RadiantTest;

TEST_F(MapExportTest, exportSelectedWithFormat)
{
    createAndSelectSingleBrush();

    auto format = GlobalMapFormatManager().getMapFormatByName(map::PORTABLE_MAP_FORMAT_NAME);

    std::ostringstream output;
    GlobalMapModule().exportSelected(output, format);

    output.flush();

    // Minimal assertion: we got a string that appears to start like an XML document
    algorithm::assertStringIsMapxFile(output.str());
}

TEST_F(MapExportTest, exportSelectedDoesNotSendMessages)
{
    createAndSelectSingleBrush();

    auto format = GlobalMapFormatManager().getMapFormatByName(map::PORTABLE_MAP_FORMAT_NAME);

    bool messageReceived = false;

    // Subscribe to the messages potentially sent by the MapExporter
    auto listener = GlobalRadiantCore().getMessageBus().addListener(
        radiant::IMessage::Type::MapFileOperation,
        radiant::TypeListener<map::FileOperation>([&](map::FileOperation& msg)
    { 
        messageReceived = true; 
    }));

    std::ostringstream output;
    GlobalMapModule().exportSelected(output, format);
    output.flush();

    GlobalRadiantCore().getMessageBus().removeListener(listener);

    EXPECT_FALSE(messageReceived) << "Received a FileOperation message while exporting the selection";
}

// Exports a default cuboid brush with its texture fitted 1x1 to a temporary path 
// using the format for the given game type. Returns the exported map text.
std::string exportDefaultBrushUsingFormat(const std::string& gameType, const std::string& exportPath)
{
    auto brush = algorithm::createCuboidBrush(GlobalMapModule().findOrInsertWorldspawn(),
        AABB(Vector3(0, 0, 0), Vector3(64, 128, 256)), "textures/darkmod/numbers/1");

    for (auto i = 0; i < Node_getIBrush(brush)->getNumFaces(); ++i)
    {
        Node_getIBrush(brush)->getFace(i).fitTexture(1, 1);;
    }

    auto format = GlobalMapFormatManager().getMapFormatForGameType(gameType, os::getExtension(exportPath));

    EXPECT_FALSE(fs::exists(exportPath)) << "File already exists";

    FileSelectionHelper helper(exportPath, format);
    GlobalCommandSystem().executeCommand("ExportMap");

    EXPECT_TRUE(fs::exists(exportPath)) << "File still doesn't exist";

    std::ifstream tempFile(exportPath);
    std::stringstream content;
    content << tempFile.rdbuf();

    return content.str();
}

TEST_F(MapExportTest, exportDoom3Brush)
{
    fs::path tempPath = _context.getTemporaryDataPath();
    tempPath /= "brushexport.map";

    auto text = exportDefaultBrushUsingFormat("doom3", tempPath.string());

    auto brushTextIndex = text.find(R"BRUSH(// primitive 0
{
brushDef3
{
( 1 0 0 -64 ) ( ( 0.00390625 0 0.5 ) ( 0 0.001953125 0.5 ) ) "textures/darkmod/numbers/1" 0 0 0
( -1 0 0 -64 ) ( ( 0.00390625 0 0.5 ) ( 0 0.001953125 0.5 ) ) "textures/darkmod/numbers/1" 0 0 0
( 0 1 0 -128 ) ( ( 0.0078125 0 0.5 ) ( 0 0.001953125 0.5 ) ) "textures/darkmod/numbers/1" 0 0 0
( 0 -1 0 -128 ) ( ( 0.0078125 0 0.5 ) ( 0 0.001953125 0.5 ) ) "textures/darkmod/numbers/1" 0 0 0
( 0 0 1 -256 ) ( ( 0.00390625 0 0.5 ) ( 0 0.0078125 0.5 ) ) "textures/darkmod/numbers/1" 0 0 0
( 0 0 -1 -256 ) ( ( 0.00390625 0 0.5 ) ( 0 0.0078125 0.5 ) ) "textures/darkmod/numbers/1" 0 0 0
}
})BRUSH");

    EXPECT_NE(brushTextIndex, std::string::npos) << "Could not locate the exported brush in the expected format";
}

TEST_F(MapExportTest, exportQuake3Brush)
{
    fs::path tempPath = _context.getTemporaryDataPath();
    tempPath /= "brushexport.map";

    auto text = exportDefaultBrushUsingFormat("quake3", tempPath.string());

    // Quake 3 format (after #5711) should be writing the old brushDef format without
    // brushDef header, using the shift/scale/rotation tex def syntax
    auto brushTextIndex = text.find(R"BRUSH(// brush 0
{
( 64 128 -256 ) ( 64 -128 256 ) ( 64 128 256 ) darkmod/numbers/1 32 32 0 4 8 0 0 0
( -64 -128 -256 ) ( -64 128 256 ) ( -64 -128 256 ) darkmod/numbers/1 32 32 0 4 8 0 0 0
( -64 128 -256 ) ( 64 128 256 ) ( -64 128 256 ) darkmod/numbers/1 32 32 0 2 8 0 0 0
( 64 -128 -256 ) ( -64 -128 256 ) ( 64 -128 256 ) darkmod/numbers/1 32 32 0 2 8 0 0 0
( -64 128 256 ) ( 64 -128 256 ) ( -64 -128 256 ) darkmod/numbers/1 32 32 0 4 2 0 0 0
( -64 128 -256 ) ( 64 -128 -256 ) ( 64 128 -256 ) darkmod/numbers/1 32 32 0 4 2 0 0 0
})BRUSH");

    EXPECT_NE(brushTextIndex, std::string::npos) << "Could not locate the exported brush in the expected format";
}

TEST_F(MapExportTest, exportQuake3AlternateBrush)
{
    fs::path tempPath = _context.getTemporaryDataPath();
    tempPath /= "brushexport.map";

    auto text = exportDefaultBrushUsingFormat("quake3alternate", tempPath.string());

    // Quake 3 alternate format is  writing the newer brushDef format including the brushDef keyword
    auto brushTextIndex = text.find(R"BRUSH(// brush 0
{
brushDef
{
( 64 128 -256 ) ( 64 -128 256 ) ( 64 128 256 ) ( ( 0.00390625 0 0.5 ) ( 0 0.001953125 0.5 ) ) darkmod/numbers/1 0 0 0
( -64 -128 -256 ) ( -64 128 256 ) ( -64 -128 256 ) ( ( 0.00390625 0 0.5 ) ( 0 0.001953125 0.5 ) ) darkmod/numbers/1 0 0 0
( -64 128 -256 ) ( 64 128 256 ) ( -64 128 256 ) ( ( 0.0078125 0 0.5 ) ( 0 0.001953125 0.5 ) ) darkmod/numbers/1 0 0 0
( 64 -128 -256 ) ( -64 -128 256 ) ( 64 -128 256 ) ( ( 0.0078125 0 0.5 ) ( 0 0.001953125 0.5 ) ) darkmod/numbers/1 0 0 0
( -64 128 256 ) ( 64 -128 256 ) ( -64 -128 256 ) ( ( 0.00390625 0 0.5 ) ( 0 0.0078125 0.5 ) ) darkmod/numbers/1 0 0 0
( -64 128 -256 ) ( 64 -128 -256 ) ( 64 128 -256 ) ( ( 0.00390625 0 0.5 ) ( 0 0.0078125 0.5 ) ) darkmod/numbers/1 0 0 0
}
})BRUSH");

    EXPECT_NE(brushTextIndex, std::string::npos) << "Could not locate the exported brush in the expected format";
}

TEST_F(MapExportTest, exportQuake4Brush)
{
    fs::path tempPath = _context.getTemporaryDataPath();
    tempPath /= "brushexport.map";

    auto text = exportDefaultBrushUsingFormat("quake4", tempPath.string());

    // Quake 4 format is like Doom 3, just without the additional flag numbers at the end of each face
    auto brushTextIndex = text.find(R"BRUSH(// primitive 0
{
brushDef3
{
( 1 0 0 -64 ) ( ( 0.00390625 0 0.5 ) ( 0 0.001953125 0.5 ) ) "textures/darkmod/numbers/1" 
( -1 0 0 -64 ) ( ( 0.00390625 0 0.5 ) ( 0 0.001953125 0.5 ) ) "textures/darkmod/numbers/1" 
( 0 1 0 -128 ) ( ( 0.0078125 0 0.5 ) ( 0 0.001953125 0.5 ) ) "textures/darkmod/numbers/1" 
( 0 -1 0 -128 ) ( ( 0.0078125 0 0.5 ) ( 0 0.001953125 0.5 ) ) "textures/darkmod/numbers/1" 
( 0 0 1 -256 ) ( ( 0.00390625 0 0.5 ) ( 0 0.0078125 0.5 ) ) "textures/darkmod/numbers/1" 
( 0 0 -1 -256 ) ( ( 0.00390625 0 0.5 ) ( 0 0.0078125 0.5 ) ) "textures/darkmod/numbers/1" 
}
})BRUSH");

    EXPECT_NE(brushTextIndex, std::string::npos) << "Could not locate the exported brush in the expected format";
}

}
