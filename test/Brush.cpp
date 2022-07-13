#include "RadiantTest.h"

#include "ibrush.h"
#include "imap.h"
#include "iselection.h"
#include "itransformable.h"
#include "scenelib.h"
#include "math/Quaternion.h"
#include "algorithm/Scene.h"
#include "algorithm/Primitives.h"
#include "math/Vector3.h"
#include "os/path.h"
#include "testutil/FileSelectionHelper.h"

namespace test
{

// Fuzzy equality assertion for Plane3
void expectNear(const Plane3& p1, const Plane3& p2, double epsilon)
{
    EXPECT_NEAR(p1.normal().x(), p2.normal().x(), epsilon);
    EXPECT_NEAR(p1.normal().y(), p2.normal().y(), epsilon);
    EXPECT_NEAR(p1.normal().z(), p2.normal().z(), epsilon);
    EXPECT_NEAR(p1.dist(), p2.dist(), epsilon);
}

using Quake3BrushTest = RadiantTest;

class BrushTest: public RadiantTest
{
protected:
    scene::INodePtr _brushNode;

    void testFacePlane(const std::function<bool(const IBrushNodePtr&)>& functor)
    {
        auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();

        _brushNode = GlobalBrushCreator().createBrush();
        worldspawn->addChildNode(_brushNode);

        GlobalSelectionSystem().setSelectedAll(false);
        Node_setSelected(_brushNode, true);

        const double size = 15;
        Vector3 startPos(-size, -size, -size);
        Vector3 endPos(size, size, size);
        GlobalCommandSystem().executeCommand("ResizeSelectedBrushesToBounds",
                                             {startPos, endPos, std::string("shader")});

        auto result = functor(std::dynamic_pointer_cast<IBrushNode>(_brushNode));

        scene::removeNodeFromParent(_brushNode);
        _brushNode.reset();

        EXPECT_TRUE(result) << "Test failed to perform anything.";
    }
};

inline bool isSane(double value)
{
    return !std::isnan(value) && !std::isinf(value);
}

inline bool isSane(const Matrix3& matrix)
{
    return isSane(matrix.xx()) && isSane(matrix.xy()) && isSane(matrix.xz()) &&
        isSane(matrix.yx()) && isSane(matrix.yy()) && isSane(matrix.yz()) &&
        isSane(matrix.zx()) && isSane(matrix.zy()) && isSane(matrix.zz());
}

TEST_F(BrushTest, FitTextureWithZeroScale)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();

    auto brushNode = GlobalBrushCreator().createBrush();
    worldspawn->addChildNode(brushNode);

    GlobalSelectionSystem().setSelectedAll(false);
    Node_setSelected(brushNode, true);

    const double size = 15;
    Vector3 startPos(-size, -size, -size);
    Vector3 endPos(size, size, size);
    GlobalCommandSystem().executeCommand("ResizeSelectedBrushesToBounds",
                                         {startPos, endPos, std::string("shader")});

    GlobalSelectionSystem().setSelectedAll(false);

    auto brush = std::dynamic_pointer_cast<IBrushNode>(brushNode);
    brush->getIBrush().evaluateBRep();

    // Apply a texdef with a 0 scale component to the first face
    ShiftScaleRotation scr;

    scr.shift[0] = 0.0f;
    scr.shift[1] = 0.0f;
    scr.scale[0] = 1.0f;
    scr.scale[1] = 0.0f; // zero scale
    scr.rotate = 0.0f;

    auto& face = brush->getIBrush().getFace(0);

    face.setShiftScaleRotation(scr);

    auto matrix = face.getProjectionMatrix();
    EXPECT_FALSE(isSane(matrix)); // 5th matrix component is INF at the least

    // Now fit the texture
    face.fitTexture(1, 1);

    matrix = face.getProjectionMatrix();

    // The whole matrix should be sane now
    EXPECT_TRUE(isSane(matrix)) << "Texture Projection Matrix is not sane after fitting";

    scene::removeNodeFromParent(brushNode);
}

TEST_F(BrushTest, FacePlaneRotateWithMatrix)
{
    testFacePlane([this](const IBrushNodePtr& brush)
    {
        // Get the plane facing down the x axis and check it
        for (std::size_t i = 0; i < brush->getIBrush().getNumFaces(); ++i)
        {
            auto& face = brush->getIBrush().getFace(i);

            if (math::isParallel(face.getPlane3().normal(), Vector3(1, 0, 0)))
            {
                Plane3 orig = face.getPlane3();

                // Transform the plane with a rotation matrix
                const double ANGLE = 2.0;
                Matrix4 rot = Matrix4::getRotation(Vector3(0, 1, 0), ANGLE);

                scene::node_cast<ITransformable>(_brushNode)->setRotation(
                    Quaternion::createForY(-ANGLE)
                );
                scene::node_cast<ITransformable>(_brushNode)->freezeTransform();

                double EPSILON = 0.001;
                EXPECT_NE(face.getPlane3(), orig);
                expectNear(face.getPlane3(), orig.transformed(rot), EPSILON);
                EXPECT_NEAR(face.getPlane3().normal().getLength(), 1, EPSILON);

                return true;
            }
        }

        return false;
    });
}

TEST_F(BrushTest, FacePlaneTranslate)
{
    testFacePlane([this](const IBrushNodePtr& brush)
    {
        // Get the plane facing down the x axis and check it
        for (std::size_t i = 0; i < brush->getIBrush().getNumFaces(); ++i)
        {
            auto& face = brush->getIBrush().getFace(i);

            if (math::isParallel(face.getPlane3().normal(), Vector3(0, 1, 0)))
            {
                Plane3 orig = face.getPlane3();

                // Translate in the Y direction
                const Vector3 translation(0, 3, 0);

                scene::node_cast<ITransformable>(_brushNode)->setTranslation(translation);
                scene::node_cast<ITransformable>(_brushNode)->freezeTransform();

                EXPECT_NE(face.getPlane3(), orig);
                EXPECT_EQ(face.getPlane3().normal(), orig.normal());
                EXPECT_EQ(face.getPlane3().dist(), orig.dist() + translation.y());
                EXPECT_NEAR(face.getPlane3().normal().getLength(), 1, 0.001);

                return true;
            }
        }

        return false;
    });
}

// Load a brush with one vertex at 0,0,0, and an identity shift/scale/rotation texdef
TEST_F(Quake3BrushTest, LoadBrushWithIdentityTexDef)
{
    std::string mapPath = "maps/quake3maps/brush_no_transform.map";
    GlobalCommandSystem().executeCommand("OpenMap", mapPath);

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    EXPECT_EQ(algorithm::getChildCount(worldspawn), 1) << "Scene has not exactly 1 brush";

    // Check that we have exactly one brush loaded
    auto brushNode = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/a_1024x512");
    EXPECT_TRUE(brushNode && brushNode->getNodeType() == scene::INode::Type::Brush) << "Couldn't locate the test brush";

    auto face = algorithm::findBrushFaceWithNormal(Node_getIBrush(brushNode), Vector3(0, 0, 1));
    EXPECT_TRUE(face != nullptr) << "No brush plane is facing upwards?";

    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64,  0, 64), Vector2(0.0625, 0)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3( 0,  0, 64), Vector2(0, 0)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3( 0, 64, 64), Vector2(0, -0.125)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, 64, 64), Vector2(0.0625, -0.125)));

    face = algorithm::findBrushFaceWithNormal(Node_getIBrush(brushNode), Vector3(0, 0, -1));
    EXPECT_TRUE(face != nullptr) << "No brush plane is facing down?";

    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(0, 0, 0), Vector2(0, 0)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, 0, 0), Vector2(0.0625, 0)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, 64, 0), Vector2(0.0625, -0.125)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(0, 64, 0), Vector2(0, -0.125)));

    face = algorithm::findBrushFaceWithNormal(Node_getIBrush(brushNode), Vector3(0, -1, 0));
    EXPECT_TRUE(face != nullptr) << "No brush plane with normal 0,-1,0?";

    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, 0, 0), Vector2(0.0625, 0)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(0, 0, 0), Vector2(0, 0)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(0, 0, 64), Vector2(0, -0.125)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, 0, 64), Vector2(0.0625, -0.125)));

    face = algorithm::findBrushFaceWithNormal(Node_getIBrush(brushNode), Vector3(0, 1, 0));
    EXPECT_TRUE(face != nullptr) << "No brush plane with normal 0,1,0?";

    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(0, 64, 0), Vector2(0, 0)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, 64, 0), Vector2(0.0625, 0)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, 64, 64), Vector2(0.0625, -0.125)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(0, 64, 64), Vector2(0, -0.125)));

    face = algorithm::findBrushFaceWithNormal(Node_getIBrush(brushNode), Vector3(1, 0, 0));
    EXPECT_TRUE(face != nullptr) << "No brush plane with normal 1,0,0?";

    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, 64, 0), Vector2(0.0625, 0)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, 0, 0), Vector2(0, 0)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, 0, 64), Vector2(0, -0.125)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, 64, 64), Vector2(0.0625, -0.125)));

    face = algorithm::findBrushFaceWithNormal(Node_getIBrush(brushNode), Vector3(-1, 0, 0));
    EXPECT_TRUE(face != nullptr) << "No brush plane with normal -1,0,0?";

    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(0, 0, 0), Vector2(0, 0)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(0, 64, 0), Vector2(0.0625, 0)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(0, 64, 64), Vector2(0.0625, -0.125)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(0, 0, 64), Vector2(0, -0.125)));
}

// Load an axis-aligned brush somewhere at (-600 1000 56) and some shift/scale/rotation values
TEST_F(Quake3BrushTest, LoadAxisAlignedBrushWithTransform)
{
    std::string mapPath = "maps/quake3maps/brush_with_transform.map";
    GlobalCommandSystem().executeCommand("OpenMap", mapPath);

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    EXPECT_EQ(algorithm::getChildCount(worldspawn), 1) << "Scene has not exactly 1 brush";

    // Check that we have exactly one brush loaded
    auto brushNode = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/a_1024x512");
    EXPECT_TRUE(brushNode && brushNode->getNodeType() == scene::INode::Type::Brush) << "Couldn't locate the test brush";

    auto face = algorithm::findBrushFaceWithNormal(Node_getIBrush(brushNode), Vector3(0, 0, 1));
    EXPECT_TRUE(face != nullptr) << "No brush plane is facing upwards?";

    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-624, 800, 64), Vector2(5, 13)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-688, 800, 64), Vector2(5.5, 13)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-688, 1024, 64), Vector2(5.5, 16.5)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-624, 1024, 64), Vector2(5, 16.5)));
}

// This loads the same brush as in the LoadAxisAlignedBrushWithTransform test
// and stores it again using the Quake3 brush format
TEST_F(Quake3BrushTest, SaveAxisAlignedBrushWithTransform)
{
    std::string mapPath = "maps/quake3maps/brush_with_transform.map";
    GlobalCommandSystem().executeCommand("OpenMap", mapPath);

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    EXPECT_EQ(algorithm::getChildCount(worldspawn), 1) << "Scene has not exactly 1 brush";

    fs::path tempPath = _context.getTemporaryDataPath();
    tempPath /= "brushexport.map";

    auto format = GlobalMapFormatManager().getMapFormatForGameType("quake3", os::getExtension(tempPath.string()));

    EXPECT_FALSE(fs::exists(tempPath)) << "File already exists: " << tempPath;

    FileSelectionHelper helper(tempPath.string(), format);
    GlobalCommandSystem().executeCommand("ExportMap");

    EXPECT_TRUE(fs::exists(tempPath)) << "File still doesn't exist: " << tempPath;

    std::ifstream tempFile(tempPath);
    std::stringstream content;
    content << tempFile.rdbuf();

    auto savedContent = content.str();

    // This is checking the actual face string as written by the LegacyBrushDefExporter
    // including whitespace and all the syntax details
    // The incoming brush had a rotation of 180 degrees and a positive scale
    // the map exporter code will re-calculate that and spit out 0 rotation and a negative scale.
    // The incoming also had the plane points picked from the middle of the brush edge,
    // the DR exporter is using the winding vertices as points
    constexpr const char* const expectedBrushFace =
        "( -688 1024 64 ) ( -624 800 64 ) ( -688 800 64 ) a_1024x512 128 256 0 -0.125 -0.125 134217728 0 0";

    EXPECT_NE(savedContent.find(expectedBrushFace), std::string::npos) <<
        "Couldn't locate the brush face " << expectedBrushFace << "\n, Saved Content is:\n" << savedContent;
}

#if 0
// This test case is not working since the Q3 texture projection mechanics
// are different from idTech4, so DR cannot render angled faces as they would
// appear in a Q3 engine.
TEST_F(Quake3BrushTest, TextureOnAngledBrush)
{
    std::string mapPath = "maps/quake3maps/angled_brush.map";
    GlobalCommandSystem().executeCommand("OpenMap", mapPath);

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    EXPECT_EQ(algorithm::getChildCount(worldspawn), 1) << "Scene has not exactly 1 brush";

    // Check that we have exactly one brush loaded
    auto brushNode = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/a_1024x512");
    EXPECT_TRUE(brushNode && brushNode->getNodeType() == scene::INode::Type::Brush) << "Couldn't locate the test brush";

    auto face = algorithm::findBrushFaceWithNormal(Node_getIBrush(brushNode), Vector3(0, -0.351123, +0.936329));
    EXPECT_TRUE(face != nullptr) << "Couldn't find the upwards facing brush face?";

    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-624, 1040, 64), Vector2(5, 16.5)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-688, 1040, 64), Vector2(5.5, 16.5)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-688, 1168, 112), Vector2(5.5, 18.5)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-624, 1168, 112), Vector2(5, 18.5)));
}
#endif

inline void checkFaceNormalAndShader(IBrush* brush, int faceIndex, const Vector3& expectedNormal, const std::string& expectedMaterial)
{
    EXPECT_TRUE(math::isNear(brush->getFace(faceIndex).getPlane3().normal(), expectedNormal, 0.01)) <<
        "Face " << faceIndex << " failed the normal check";
    EXPECT_EQ(brush->getFace(faceIndex).getShader(), expectedMaterial) << "Face " << faceIndex << " doesn't have the expected material";
}

// Create a defined brush with a certain AABB for the rotation tests
// Face with (0,0,1) will have textures/numbers/10
// Face with (0,1,0) will have textures/numbers/11
inline scene::INodePtr createBrushForRotationTest()
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brushNode = algorithm::createCuboidBrush(worldspawn, AABB({ 192, 8, 48 }, { 48, 8, 50 }), "textures/numbers/1");
    Node_setSelected(brushNode, true);

    auto brush = Node_getIBrush(brushNode);

    // Set up two faces with defined materials to later detect how the brush has been rotated
    algorithm::findBrushFaceWithNormal(brush, { 0, 0, 1 })->setShader("textures/numbers/10");
    algorithm::findBrushFaceWithNormal(brush, { 0, 1, 0 })->setShader("textures/numbers/11");

    // Check the faces explicitly
    checkFaceNormalAndShader(brush, 0, { 1, 0, 0 }, "textures/numbers/1");
    checkFaceNormalAndShader(brush, 1, {-1, 0, 0 }, "textures/numbers/1");
    checkFaceNormalAndShader(brush, 2, { 0, 1, 0 }, "textures/numbers/11");
    checkFaceNormalAndShader(brush, 3, { 0,-1, 0 }, "textures/numbers/1");
    checkFaceNormalAndShader(brush, 4, { 0, 0, 1 }, "textures/numbers/10");
    checkFaceNormalAndShader(brush, 5, { 0, 0,-1 }, "textures/numbers/1");

    return brushNode;
}

// #5770: Brushes disappear due to transformation being applied twice while freezing the brush node (first face is transformed twice)
TEST_F(BrushTest, RotateBrushZ90Degrees)
{
    auto brushNode = createBrushForRotationTest();

    GlobalCommandSystem().executeCommand("RotateSelectionZ");

    auto brush = Node_getIBrush(brushNode);

    // Check the faces explicitly, normals should have been rotated, order of faces unchanged
    checkFaceNormalAndShader(brush, 0, { 0,-1, 0 }, "textures/numbers/1");
    checkFaceNormalAndShader(brush, 1, { 0, 1, 0 }, "textures/numbers/1");
    checkFaceNormalAndShader(brush, 2, { 1, 0, 0 }, "textures/numbers/11");
    checkFaceNormalAndShader(brush, 3, {-1, 0, 0 }, "textures/numbers/1");
    checkFaceNormalAndShader(brush, 4, { 0, 0, 1 }, "textures/numbers/10");
    checkFaceNormalAndShader(brush, 5, { 0, 0,-1 }, "textures/numbers/1");
}

TEST_F(BrushTest, RotateBrushX90Degrees)
{
    auto brushNode = createBrushForRotationTest();

    GlobalCommandSystem().executeCommand("RotateSelectionX");

    auto brush = Node_getIBrush(brushNode);

    // Check the faces explicitly, normals should have been rotated, order of faces unchanged
    checkFaceNormalAndShader(brush, 0, { 1, 0, 0 }, "textures/numbers/1");
    checkFaceNormalAndShader(brush, 1, {-1, 0, 0 }, "textures/numbers/1");
    checkFaceNormalAndShader(brush, 2, { 0, 0,-1 }, "textures/numbers/11");
    checkFaceNormalAndShader(brush, 3, { 0, 0, 1 }, "textures/numbers/1");
    checkFaceNormalAndShader(brush, 4, { 0, 1, 0 }, "textures/numbers/10");
    checkFaceNormalAndShader(brush, 5, { 0,-1, 0 }, "textures/numbers/1");
}

TEST_F(BrushTest, RotateBrushY90Degrees)
{
    auto brushNode = createBrushForRotationTest();

    GlobalCommandSystem().executeCommand("RotateSelectionY");

    auto brush = Node_getIBrush(brushNode);

    // Check the faces explicitly, normals should have been rotated, order of faces unchanged
    checkFaceNormalAndShader(brush, 0, { 0, 0,-1 }, "textures/numbers/1");
    checkFaceNormalAndShader(brush, 1, { 0, 0, 1 }, "textures/numbers/1");
    checkFaceNormalAndShader(brush, 2, { 0, 1, 0 }, "textures/numbers/11");
    checkFaceNormalAndShader(brush, 3, { 0,-1, 0 }, "textures/numbers/1");
    checkFaceNormalAndShader(brush, 4, { 1, 0, 0 }, "textures/numbers/10");
    checkFaceNormalAndShader(brush, 5, {-1, 0, 0 }, "textures/numbers/1");
}

// #5942: Missing brushes when opening alphalabs1 from vanilla Doom 3
TEST_F(BrushTest, LoadBrushWithDuplicatePlanes)
{
    std::string mapPath = "maps/brush_with_duplicate_planes.map";
    GlobalCommandSystem().executeCommand("OpenMap", mapPath);

    // Check that we have exactly one brush loaded
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brushNode = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/common/caulk");
    EXPECT_TRUE(brushNode && brushNode->getNodeType() == scene::INode::Type::Brush) << "Couldn't locate the test brush";

    auto brush = Node_getIBrush(brushNode);
    brush->evaluateBRep();

    EXPECT_TRUE(brush->hasContributingFaces()) << "Brush should not be degenerate";
    EXPECT_EQ(brush->getNumFaces(), 6) << "Brush should have 6 faces after map loading, even though it has been declared with 9";

    // The second duplicates should have been removed, they were textured with caulk2
    // Therefore all 6 faces should have the caulk texture exclusively
    for (auto i = 0; i < brush->getNumFaces(); ++i)
    {
        auto& face = brush->getFace(i);

        EXPECT_EQ(face.getShader(), "textures/common/caulk") << "Face " << i << " should be textured with caulk";
    }
}

}
