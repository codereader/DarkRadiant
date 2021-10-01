#include "RadiantTest.h"

#include "algorithm/Scene.h"
#include "algorithm/Primitives.h"
#include "scenelib.h"

namespace test
{

using TextureManipulationTest = RadiantTest;

TEST_F(TextureManipulationTest, FaceRotateTexDef)
{
    std::string mapPath = "maps/simple_brushes.map";
    GlobalCommandSystem().executeCommand("OpenMap", mapPath);

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();

    // Find the brush that is centered at origin
    auto brushNode = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/2");
    EXPECT_TRUE(brushNode && brushNode->getNodeType() == scene::INode::Type::Brush) << "Couldn't locate the test brush";

    // Pick a few faces and run the algorithm against it, checking hardcoded results

    // Facing 0,0,1
    auto face = algorithm::findBrushFaceWithNormal(Node_getIBrush(brushNode), Vector3(0, 0, 1));

    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, -64, -160), Vector2(0, 1)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-64, -64, -160), Vector2(0, 0)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-64, 64, -160), Vector2(1, 0)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, 64, -160), Vector2(1, 1)));

    face->rotateTexdef(15); // degrees

    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, -64, -160), Vector2(-0.112372, 0.853553)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-64, -64, -160), Vector2(0.146447, -0.112372)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-64, 64, -160), Vector2(1.11237, 0.146447)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, 64, -160), Vector2(0.853553, 1.11237)));

    // Facing 1,0,0
    face = algorithm::findBrushFaceWithNormal(Node_getIBrush(brushNode), Vector3(1, 0, 0));

    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, -64, -288), Vector2(0, 65)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, -64, -160), Vector2(0, 64)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, 64, -160), Vector2(1, 64)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, 64, -288), Vector2(1, 65)));

    face->rotateTexdef(15); // degrees

    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, -64, -288), Vector2(-0.112372, 64.8536)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, -64, -160), Vector2(0.146447, 63.8876)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, 64, -160), Vector2(1.11237, 64.1464)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, 64, -288), Vector2(0.853553, 65.1124)));

    // Facing 0,0,-1
    face = algorithm::findBrushFaceWithNormal(Node_getIBrush(brushNode), Vector3(0, 0, -1));

    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-64, -64, -288), Vector2(0, 1)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, -64, -288), Vector2(0, 0)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, 64, -288), Vector2(1, 0)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-64, 64, -288), Vector2(1, 1)));

    face->rotateTexdef(15); // degrees

    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-64, -64, -288), Vector2(-0.112372, 0.853553)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, -64, -288), Vector2(0.146447, -0.112372)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, 64, -288), Vector2(1.11237, 0.146447)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-64, 64, -288), Vector2(0.853553, 1.11237)));

    // Facing 0,-1,0, this time rotate -15 degrees
    face = algorithm::findBrushFaceWithNormal(Node_getIBrush(brushNode), Vector3(0, -1, 0));

    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-64, -64, -160), Vector2(0, 64)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, -64, -160), Vector2(1, 64)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, -64, -288), Vector2(1, 65)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-64, -64, -288), Vector2(0, 65)));

    face->rotateTexdef(-15); // degrees

    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-64, -64, -160), Vector2(-0.112372, 64.1464)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, -64, -160), Vector2(0.853553, 63.8876)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(64, -64, -288), Vector2(1.11237, 64.8536)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-64, -64, -288), Vector2(0.146447, 65.1124)));
}

namespace
{

void performFaceFlipTest(int axis)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush = algorithm::createCubicBrush(worldspawn, Vector3(0, 256, 256), "textures/numbers/1");
    scene::addNodeToContainer(brush, worldspawn);
    Node_setSelected(brush, true);

    auto faceUp = algorithm::findBrushFaceWithNormal(Node_getIBrush(brush), Vector3(0, 0, 1));

    std::vector<Vector2> oldTexCoords;
    for (const auto& vertex : faceUp->getWinding())
    {
        oldTexCoords.push_back(vertex.texcoord);
    }

    auto cmd = axis == 0 ? "FlipTextureX" : "FlipTextureY";
    GlobalCommandSystem().executeCommand(cmd);

    auto faceUvBounds = algorithm::getTextureSpaceBounds(*faceUp);

    // Every face vertex should have been flipped about the bounds origin
    auto old = oldTexCoords.begin();
    for (const auto& vertex : faceUp->getWinding())
    {
        // Calculate the mirrored coordinate
        auto expectedTexcoord = *(old++);
        expectedTexcoord[axis] = 2 * faceUvBounds.origin[axis] - expectedTexcoord[axis];

        EXPECT_EQ(vertex.texcoord.x(), expectedTexcoord.x()) << "Mirrored vertex should be at " << expectedTexcoord;
        EXPECT_EQ(vertex.texcoord.y(), expectedTexcoord.y()) << "Mirrored vertex should be at " << expectedTexcoord;
    }
}

void performPatchFlipTest(int axis)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto patchNode = algorithm::createPatchFromBounds(worldspawn, AABB(Vector3(4, 50, 60), Vector3(64, 128, 256)), "textures/numbers/1");

    auto patch = Node_getIPatch(patchNode);
    patch->scaleTextureNaturally();
    patch->controlPointsChanged();

    Node_setSelected(patchNode, true);

    // Get the texture space bounds of this patch
    auto patchBounds = algorithm::getTextureSpaceBounds(*patch);
    auto bounds = patchBounds;
    bounds.extents *= 1.2f;

    std::vector<Vector2> oldTexCoords;
    algorithm::foreachPatchVertex(*patch, [&](const PatchControl& ctrl) { oldTexCoords.push_back(ctrl.texcoord); });

    auto cmd = axis == 0 ? "FlipTextureX" : "FlipTextureY";
    GlobalCommandSystem().executeCommand(cmd);

    // Every changed vertex should have been flipped about the bounds origin
    algorithm::expectVerticesHaveBeenFlipped(axis, *patch, oldTexCoords, { patchBounds.origin.x(), patchBounds.origin.y() });
}

}

TEST_F(TextureManipulationTest, FaceFlipS)
{
    performFaceFlipTest(0);
}

TEST_F(TextureManipulationTest, FaceFlipT)
{
    performFaceFlipTest(1);
}

TEST_F(TextureManipulationTest, PatchFlipS)
{
    performPatchFlipTest(0);
}

TEST_F(TextureManipulationTest, PatchFlipT)
{
    performPatchFlipTest(1);
}

}
