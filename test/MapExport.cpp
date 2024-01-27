#include "RadiantTest.h"

#include "imap.h"
#include "imapformat.h"
#include "ibrush.h"
#include "math/Plane3.h"
#include "math/Matrix3.h"
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

    return algorithm::loadFileToString(exportPath);
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

struct XmlFace
{
    Plane3 plane;
    Matrix3 textureMatrix;
    std::string material;
    unsigned int contentsFlag;

    static XmlFace ParseFromNode(const xml::Node& node)
    {
        XmlFace face;

        face.material = node.getNamedChildren("material")[0].getAttributeValue("name");

        auto planeNode = node.getNamedChildren("plane")[0];

        face.plane.normal().x() = string::to_float(planeNode.getAttributeValue("x"));
        face.plane.normal().y() = string::to_float(planeNode.getAttributeValue("y"));
        face.plane.normal().z() = string::to_float(planeNode.getAttributeValue("z"));
        face.plane.dist() = -string::to_float(planeNode.getAttributeValue("d")); // negate d

        auto texProjTag = node.getNamedChildren("textureProjection")[0];

        face.textureMatrix.xx() = string::to_float(texProjTag.getAttributeValue("xx"));
        face.textureMatrix.yx() = string::to_float(texProjTag.getAttributeValue("yx"));
        face.textureMatrix.zx() = string::to_float(texProjTag.getAttributeValue("tx"));
        face.textureMatrix.xy() = string::to_float(texProjTag.getAttributeValue("xy"));
        face.textureMatrix.yy() = string::to_float(texProjTag.getAttributeValue("yy"));
        face.textureMatrix.zy() = string::to_float(texProjTag.getAttributeValue("ty"));

        face.contentsFlag = string::convert<unsigned int>(node.getNamedChildren("contentsFlag")[0].getAttributeValue("value"));

        return face;
    }
};

inline bool hasFaceWithProperties(const std::vector<XmlFace>& faces, const std::string& material, unsigned int contentsFlag,
    const Plane3& plane, double xx, double yx, double tx, double xy, double yy, double ty)
{
    constexpr double Epsilon = 0.001;
    
    for (const auto& face : faces)
    {
        if (face.material != material) continue;

        if (face.contentsFlag != contentsFlag) continue;

        if (!math::isNear(face.plane.normal(), plane.normal(), Epsilon)) continue;
        if (std::abs(face.plane.dist() - plane.dist()) > Epsilon) continue;

        if (std::abs(face.textureMatrix.xx() - xx) > Epsilon) continue;
        if (std::abs(face.textureMatrix.yx() - yx) > Epsilon) continue;
        if (std::abs(face.textureMatrix.zx() - tx) > Epsilon) continue;
        if (std::abs(face.textureMatrix.xy() - xy) > Epsilon) continue;
        if (std::abs(face.textureMatrix.yy() - yy) > Epsilon) continue;
        if (std::abs(face.textureMatrix.zy() - ty) > Epsilon) continue;

        return true;
    }

    return false;
}

TEST_F(MapExportTest, exportDoom3BrushPortable)
{
    fs::path tempPath = _context.getTemporaryDataPath();
    tempPath /= "brushexport.mapx";

    auto text = exportDefaultBrushUsingFormat("doom3", tempPath.string());

    std::stringstream stream(text);
    xml::Document doc(stream);

    auto brush = doc.findXPath("//brush[@number='0']");
    EXPECT_EQ(brush.size(), 1) << "More than 1 brush found in XML";
    
    auto faceNodes = doc.findXPath("//brush[@number='0']/faces//face");
    EXPECT_EQ(faceNodes.size(), 6) << "Other than 6 faces brush found in XML";

    std::vector<XmlFace> faces;
    for (const auto& faceNode : faceNodes)
    {
        faces.emplace_back(XmlFace::ParseFromNode(faceNode));
    }

    EXPECT_TRUE(hasFaceWithProperties(faces, "textures/darkmod/numbers/1", 0, Plane3(1.0, 0.0, 0.0, 64), 0.003906, 0, 0.5, 0, 0.001953, 0.5));
    EXPECT_TRUE(hasFaceWithProperties(faces, "textures/darkmod/numbers/1", 0, Plane3(-1.0, 0.0, 0.0, 64), 0.003906, 0, 0.5, 0, 0.001953, 0.5));
    EXPECT_TRUE(hasFaceWithProperties(faces, "textures/darkmod/numbers/1", 0, Plane3(0, 1.0, 0.0, 128), 0.007813, 0, 0.5, 0, 0.001953, 0.5));
    EXPECT_TRUE(hasFaceWithProperties(faces, "textures/darkmod/numbers/1", 0, Plane3(0, -1.0, 0.0, 128), 0.007813, 0, 0.5, 0, 0.001953, 0.5));
    EXPECT_TRUE(hasFaceWithProperties(faces, "textures/darkmod/numbers/1", 0, Plane3(0, 0, 1.0, 256), 0.003906, 0, 0.5, 0, 0.007813, 0.5));
    EXPECT_TRUE(hasFaceWithProperties(faces, "textures/darkmod/numbers/1", 0, Plane3(0, 0, -1.0, 256), 0.003906, 0, 0.5, 0, 0.007813, 0.5));
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
( 64 128 -256 ) ( 64 -128 256 ) ( 64 128 256 ) darkmod/numbers/1 32 32 -180 -4 -8 0 0 0
( -64 -128 -256 ) ( -64 128 256 ) ( -64 -128 256 ) darkmod/numbers/1 32 32 0 -4 8 0 0 0
( -64 128 -256 ) ( 64 128 256 ) ( -64 128 256 ) darkmod/numbers/1 32 32 0 -2 8 0 0 0
( 64 -128 -256 ) ( -64 -128 256 ) ( 64 -128 256 ) darkmod/numbers/1 32 32 -180 -2 -8 0 0 0
( -64 128 256 ) ( 64 -128 256 ) ( -64 -128 256 ) darkmod/numbers/1 32 32 90 4 2 0 0 0
( -64 128 -256 ) ( 64 -128 -256 ) ( 64 128 -256 ) darkmod/numbers/1 32 32 90 4 -2 0 0 0
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

namespace
{

void runExportWithEmptyFileExtension(const std::string& temporaryDataPath,const std::string& command)
{
    auto brush = algorithm::createCuboidBrush(GlobalMapModule().findOrInsertWorldspawn(),
        AABB(Vector3(0, 0, 0), Vector3(64, 128, 256)), "textures/darkmod/numbers/1");

    Node_setSelected(brush, true);

    fs::path tempPath = temporaryDataPath;
    tempPath /= "export_empty_file_extension";
    EXPECT_FALSE(fs::exists(tempPath)) << "File already exists";

    // Subscribe to the event asking for the target path
    auto msgSubscription = GlobalRadiantCore().getMessageBus().addListener(
        radiant::IMessage::Type::FileSelectionRequest,
        radiant::TypeListener<radiant::FileSelectionRequest>(
            [&](radiant::FileSelectionRequest& msg)
    {
        msg.setHandled(true);
        msg.setResult(radiant::FileSelectionRequest::Result
            {
                tempPath.string(),
                "" // this can happen if e.g. the *.* filter is active
            });
    }));
    GlobalCommandSystem().executeCommand(command);

    EXPECT_TRUE(fs::exists(tempPath)) << "File still doesn't exist";

    auto content = algorithm::loadFileToString(tempPath);

    EXPECT_NE(content.find("brushDef3"), std::string::npos) << "Couldn't find the brush keyword in the export";
}

}

TEST_F(MapExportTest, ExportSelectedWithEmptyFileExtension)
{
    runExportWithEmptyFileExtension(_context.getTemporaryDataPath(), "SaveSelected");
}

TEST_F(MapExportTest, ExportPrefabWithEmptyFileExtension)
{
    runExportWithEmptyFileExtension(_context.getTemporaryDataPath(), "SaveSelectedAsPrefab");
}

}
