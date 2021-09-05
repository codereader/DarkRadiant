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
            startPos, endPos, std::string("shader"));

        auto result = functor(std::dynamic_pointer_cast<IBrushNode>(_brushNode));

        scene::removeNodeFromParent(_brushNode);
        _brushNode.reset();

        EXPECT_TRUE(result) << "Test failed to perform anything.";
    }
};

inline bool isSane(const Matrix4& matrix)
{
    for (std::size_t i = 0; i < 15; ++i)
    {
        if (std::isnan(matrix[i]) || std::isinf(matrix[i]))
        {
            return false;
        }
    }

    return true;
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
        startPos, endPos, std::string("shader"));

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

                Node_getTransformable(_brushNode)->setRotation(
                    Quaternion::createForY(-ANGLE)
                );
                Node_getTransformable(_brushNode)->freezeTransform();

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

                Node_getTransformable(_brushNode)->setTranslation(translation);
                Node_getTransformable(_brushNode)->freezeTransform();

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

inline bool faceHasVertex(const IFace* face, const Vector3& expectedXYZ, const Vector2& expectedUV)
{
    return algorithm::faceHasVertex(face, [&](const WindingVertex& vertex)
    {
        return math::isNear(vertex.vertex, expectedXYZ, 0.01) && math::isNear(vertex.texcoord, expectedUV, 0.01);
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

    EXPECT_TRUE(faceHasVertex(face, Vector3(64,  0, 64), Vector2(0.0625, 0)));
    EXPECT_TRUE(faceHasVertex(face, Vector3( 0,  0, 64), Vector2(0, 0)));
    EXPECT_TRUE(faceHasVertex(face, Vector3( 0, 64, 64), Vector2(0, -0.125)));
    EXPECT_TRUE(faceHasVertex(face, Vector3(64, 64, 64), Vector2(0.0625, -0.125)));

    face = algorithm::findBrushFaceWithNormal(Node_getIBrush(brushNode), Vector3(0, 0, -1));
    EXPECT_TRUE(face != nullptr) << "No brush plane is facing down?";

    EXPECT_TRUE(faceHasVertex(face, Vector3(0, 0, 0), Vector2(0, 0)));
    EXPECT_TRUE(faceHasVertex(face, Vector3(64, 0, 0), Vector2(0.0625, 0)));
    EXPECT_TRUE(faceHasVertex(face, Vector3(64, 64, 0), Vector2(0.0625, -0.125)));
    EXPECT_TRUE(faceHasVertex(face, Vector3(0, 64, 0), Vector2(0, -0.125)));

    face = algorithm::findBrushFaceWithNormal(Node_getIBrush(brushNode), Vector3(0, -1, 0));
    EXPECT_TRUE(face != nullptr) << "No brush plane with normal 0,-1,0?";

    EXPECT_TRUE(faceHasVertex(face, Vector3(64, 0, 0), Vector2(0.0625, 0)));
    EXPECT_TRUE(faceHasVertex(face, Vector3(0, 0, 0), Vector2(0, 0)));
    EXPECT_TRUE(faceHasVertex(face, Vector3(0, 0, 64), Vector2(0, -0.125)));
    EXPECT_TRUE(faceHasVertex(face, Vector3(64, 0, 64), Vector2(0.0625, -0.125)));

    face = algorithm::findBrushFaceWithNormal(Node_getIBrush(brushNode), Vector3(0, 1, 0));
    EXPECT_TRUE(face != nullptr) << "No brush plane with normal 0,1,0?";

    EXPECT_TRUE(faceHasVertex(face, Vector3(0, 64, 0), Vector2(0, 0)));
    EXPECT_TRUE(faceHasVertex(face, Vector3(64, 64, 0), Vector2(0.0625, 0)));
    EXPECT_TRUE(faceHasVertex(face, Vector3(64, 64, 64), Vector2(0.0625, -0.125)));
    EXPECT_TRUE(faceHasVertex(face, Vector3(0, 64, 64), Vector2(0, -0.125)));

    face = algorithm::findBrushFaceWithNormal(Node_getIBrush(brushNode), Vector3(1, 0, 0));
    EXPECT_TRUE(face != nullptr) << "No brush plane with normal 1,0,0?";

    EXPECT_TRUE(faceHasVertex(face, Vector3(64, 64, 0), Vector2(0.0625, 0)));
    EXPECT_TRUE(faceHasVertex(face, Vector3(64, 0, 0), Vector2(0, 0)));
    EXPECT_TRUE(faceHasVertex(face, Vector3(64, 0, 64), Vector2(0, -0.125)));
    EXPECT_TRUE(faceHasVertex(face, Vector3(64, 64, 64), Vector2(0.0625, -0.125)));

    face = algorithm::findBrushFaceWithNormal(Node_getIBrush(brushNode), Vector3(-1, 0, 0));
    EXPECT_TRUE(face != nullptr) << "No brush plane with normal -1,0,0?";

    EXPECT_TRUE(faceHasVertex(face, Vector3(0, 0, 0), Vector2(0, 0)));
    EXPECT_TRUE(faceHasVertex(face, Vector3(0, 64, 0), Vector2(0.0625, 0)));
    EXPECT_TRUE(faceHasVertex(face, Vector3(0, 64, 64), Vector2(0.0625, -0.125)));
    EXPECT_TRUE(faceHasVertex(face, Vector3(0, 0, 64), Vector2(0, -0.125)));
}

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

    EXPECT_TRUE(faceHasVertex(face, Vector3(-624, 1040, 64), Vector2(5, 16.5)));
    EXPECT_TRUE(faceHasVertex(face, Vector3(-688, 1040, 64), Vector2(5.5, 16.5)));
    EXPECT_TRUE(faceHasVertex(face, Vector3(-688, 1168, 112), Vector2(5.5, 18.5)));
    EXPECT_TRUE(faceHasVertex(face, Vector3(-624, 1168, 112), Vector2(5, 18.5)));
}

}
