#include "RadiantTest.h"

#include <optional>
#include "itransformable.h"
#include "ishaders.h"
#include "ishaderclipboard.h"
#include "algorithm/Scene.h"
#include "algorithm/Primitives.h"
#include "scenelib.h"
#include "math/Matrix3.h"
#include "math/Quaternion.h"
#include "registry/registry.h"
#include "render/CameraView.h"
#include "algorithm/View.h"

namespace test
{

using TextureManipulationTest = RadiantTest;

TEST_F(TextureManipulationTest, RotateFace)
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

void performPatchRotateTest(bool clockwise)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto patchNode = algorithm::createPatchFromBounds(worldspawn, AABB(Vector3(4, 50, 60), Vector3(64, 128, 256)), "textures/numbers/1");

    auto patch = Node_getIPatch(patchNode);
    patch->scaleTextureNaturally();
    patch->controlPointsChanged();

    Node_setSelected(patchNode, true);

    auto bounds = algorithm::getTextureSpaceBounds(*patch);

    std::vector<Vector2> oldTexCoords;
    algorithm::foreachPatchVertex(*patch, [&](const PatchControl& ctrl) { oldTexCoords.push_back(ctrl.texcoord); });

    // Set the rotation in the registry, TexRotate will pick it up
    constexpr auto angle = 15.0f;
    registry::setValue("user/ui/textures/surfaceInspector/rotStep", angle);
    GlobalCommandSystem().executeCommand("TexRotate", { cmd::Argument(clockwise ? "1" : "-1") });

    // Rotate each texture coordinate around the patch center with a manual transform
    auto transform = Matrix3::getTranslation({ -bounds.origin.x(), -bounds.origin.y() });
    transform.premultiplyBy(Matrix3::getRotation(degrees_to_radians(clockwise ? angle : -angle)));
    transform.premultiplyBy(Matrix3::getTranslation({ bounds.origin.x(), bounds.origin.y() }));

    auto old = oldTexCoords.begin();
    algorithm::foreachPatchVertex(*patch, [&](const PatchControl& ctrl)
    {
        auto transformed = transform * (*old++);
        EXPECT_TRUE(math::isNear(transformed, ctrl.texcoord, 0.02))
            << "Transformed UV coords should be " << transformed << " but was " << ctrl.texcoord;
    });
}

}

TEST_F(TextureManipulationTest, RotatePatchClockwise)
{
    performPatchRotateTest(true); // clockwise
}

TEST_F(TextureManipulationTest, RotatePatchCounterClockwise)
{
    performPatchRotateTest(false); // clockwise
}

namespace
{

void performFaceFlipTest(int axis)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush = algorithm::createCubicBrush(worldspawn, Vector3(0, 256, 256), "textures/numbers/1");
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

TEST_F(TextureManipulationTest, FlipFaceHorizontally)
{
    performFaceFlipTest(0);
}

TEST_F(TextureManipulationTest, FlipFaceVertically)
{
    performFaceFlipTest(1);
}

TEST_F(TextureManipulationTest, FlipPatchHorizontally)
{
    performPatchFlipTest(0);
}

TEST_F(TextureManipulationTest, FlipPatchVertically)
{
    performPatchFlipTest(1);
}

namespace
{

void assumeVerticesHaveBeenScaled(const std::vector<Vector2>& oldCoords, const std::vector<Vector2>& newCoords, const Vector2& scale, const Vector2& pivot)
{
    auto oldCoord = oldCoords.begin();
    auto newCoord = newCoords.begin();

    auto transform = Matrix3::getTranslation(-pivot);
    transform.premultiplyBy(Matrix3::getScale(scale));
    transform.premultiplyBy(Matrix3::getTranslation(pivot));

    while (newCoord != newCoords.end())
    {
        auto expected = transform * (*oldCoord++);
        auto actual = *newCoord++;
        EXPECT_TRUE(math::isNear(actual, expected, 0.01)) << "Vertex should be " << expected << ", but was " << actual;
    }
}

void performFaceScaleTest(const Vector2& scale)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush = algorithm::createCubicBrush(worldspawn, Vector3(0, 256, 256), "textures/numbers/1");
    Node_setSelected(brush, true);

    auto faceUp = algorithm::findBrushFaceWithNormal(Node_getIBrush(brush), Vector3(0, 0, 1));

    std::vector<Vector2> oldTexCoords;
    for (const auto& vertex : faceUp->getWinding())
    {
        oldTexCoords.push_back(vertex.texcoord);
    }

    // The incoming scale values are absolute 1.05 == 105%, the command accepts relative values, 0.05 == 105%
    auto zeroBasedScale = scale - Vector2(1, 1);
    GlobalCommandSystem().executeCommand("TexScale", { cmd::Argument(zeroBasedScale) });

    auto faceUvBounds = algorithm::getTextureSpaceBounds(*faceUp);

    std::vector<Vector2> newTexCoords;
    for (const auto& vertex : faceUp->getWinding())
    {
        newTexCoords.push_back(vertex.texcoord);
    }

    Vector2 pivot(faceUvBounds.origin.x(), faceUvBounds.origin.y());
    assumeVerticesHaveBeenScaled(oldTexCoords, newTexCoords, scale, pivot);
}

void performPatchScaleTest(const Vector2& scale)
{
    // We create two patches, each of them should be scaled independently
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto patchNode1 = algorithm::createPatchFromBounds(worldspawn, AABB(Vector3(4, 50, 60), Vector3(64, 128, 256)), "textures/numbers/1");
    auto patchNode2 = algorithm::createPatchFromBounds(worldspawn, AABB(Vector3(4, 50, -5), Vector3(64, 128, 64)), "textures/numbers/1");

    auto patch1 = Node_getIPatch(patchNode1);
    auto patch2 = Node_getIPatch(patchNode2);
    patch1->scaleTextureNaturally();
    patch1->controlPointsChanged();
    patch2->scaleTextureNaturally();
    patch2->controlPointsChanged();

    Node_setSelected(patchNode1, true);
    Node_setSelected(patchNode2, true);

    std::vector<Vector2> oldTexCoords1;
    std::vector<Vector2> oldTexCoords2;
    algorithm::foreachPatchVertex(*patch1, [&](const PatchControl& ctrl) { oldTexCoords1.push_back(ctrl.texcoord); });
    algorithm::foreachPatchVertex(*patch2, [&](const PatchControl& ctrl) { oldTexCoords2.push_back(ctrl.texcoord); });

    // The incoming scale values are absolute 1.05 == 105%, the command accepts relative values, 0.05 == 105%
    auto zeroBasedScale = scale - Vector2(1, 1);
    GlobalCommandSystem().executeCommand("TexScale", { cmd::Argument(zeroBasedScale) });

    auto uvBounds1 = algorithm::getTextureSpaceBounds(*patch1);
    auto uvBounds2 = algorithm::getTextureSpaceBounds(*patch2);

    std::vector<Vector2> newTexCoords1;
    std::vector<Vector2> newTexCoords2;
    algorithm::foreachPatchVertex(*patch1, [&](const PatchControl& ctrl) { newTexCoords1.push_back(ctrl.texcoord); });
    algorithm::foreachPatchVertex(*patch2, [&](const PatchControl& ctrl) { newTexCoords2.push_back(ctrl.texcoord); });

    Vector2 pivot(uvBounds1.origin.x(), uvBounds1.origin.y());
    assumeVerticesHaveBeenScaled(oldTexCoords1, newTexCoords1, scale, pivot);

    Vector2 pivot2(uvBounds2.origin.x(), uvBounds2.origin.y());
    assumeVerticesHaveBeenScaled(oldTexCoords2, newTexCoords2, scale, pivot2);
}

}

TEST_F(TextureManipulationTest, ScaleFaceUniformly)
{
    performFaceScaleTest({ 1.1, 1.1 });
}

TEST_F(TextureManipulationTest, ScaleFaceNonUniformly)
{
    performFaceScaleTest({ 1.2, 0.9 });
}

TEST_F(TextureManipulationTest, ScalePatchUniformly)
{
    performPatchScaleTest({ 1.1, 1.1 });
}

TEST_F(TextureManipulationTest, ScalePatchNonUniformly)
{
    performPatchScaleTest({ 1.2, 0.9 });
}

std::vector<std::pair<const WindingVertex*, const WindingVertex*>> findSharedVertices(const IFace& face1, const IFace& face2)
{
    std::vector<std::pair<const WindingVertex*, const WindingVertex*>> vertexPairs;

    for (const auto& vertex1 : face1.getWinding())
    {
        for (const auto& vertex2 : face2.getWinding())
        {
            if (math::isNear(vertex1.vertex, vertex2.vertex, 0.01))
            {
                vertexPairs.emplace_back(std::make_pair(&vertex1, &vertex2));
            }
        }
    }

    return vertexPairs;
}

// Paste texture from the top face of a brush to the one one the front side (X)
TEST_F(TextureManipulationTest, PasteTextureToOrthogonalFace1)
{
    std::string mapPath = "maps/simple_brushes.map";
    GlobalCommandSystem().executeCommand("OpenMap", mapPath);

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brushNode = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/3");

    auto faceUp = algorithm::findBrushFaceWithNormal(Node_getIBrush(brushNode), Vector3(0, 0, 1));
    auto faceRight = algorithm::findBrushFaceWithNormal(Node_getIBrush(brushNode), Vector3(1, 0, 0));

    // Find the shared vertices of the two faces
    auto sharedVertices = findSharedVertices(*faceUp, *faceRight);
    EXPECT_EQ(sharedVertices.size(), 2) << "There should be 2 shared 3D vertices between the two faces";

    for (const auto& pair : sharedVertices)
    {
        EXPECT_FALSE(math::isNear(pair.first->texcoord, pair.second->texcoord, 0.01))
            << "Texture coordinates are the same before pasting the texture projection";
    }

    // Position the camera top-down, similar to what an XY view is seeing
    render::View viewFaceUp(true);
    algorithm::constructCameraView(viewFaceUp, brushNode->localAABB(), { 0, 0, -1 }, { -90, 0, 0 });

    SelectionVolume testFaceUp(viewFaceUp);
    GlobalShaderClipboard().pickFromSelectionTest(testFaceUp);

    EXPECT_EQ(GlobalShaderClipboard().getSourceType(), selection::IShaderClipboard::SourceType::Face)
        << "Selection test failed to select the face";

    render::View viewFaceRight(true);
    algorithm::constructCameraView(viewFaceRight, brushNode->localAABB(), { -1, 0, 0 }, { 0, 180, 0 });

    ConstructSelectionTest(viewFaceRight, selection::Rectangle::ConstructFromPoint({ 0, 0 }, { 0.1, 0.1 }));

    SelectionVolume testFaceRight(viewFaceRight);
    GlobalShaderClipboard().pasteShader(testFaceRight, selection::PasteMode::Natural, false);

    for (const auto& pair : sharedVertices)
    {
        EXPECT_TRUE(math::isNear(pair.first->texcoord, pair.second->texcoord, 0.01))
            << "Texture coordinates should be the same after pasting the texture projection";
    }
}

// Paste shader from top face of a brush to the one on the left (Y)
TEST_F(TextureManipulationTest, PasteTextureToOrthogonalFace2)
{
    std::string mapPath = "maps/simple_brushes.map";
    GlobalCommandSystem().executeCommand("OpenMap", mapPath);

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brushNode = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/4");

    auto faceUp = algorithm::findBrushFaceWithNormal(Node_getIBrush(brushNode), Vector3(0, 0, 1));
    auto faceYDown = algorithm::findBrushFaceWithNormal(Node_getIBrush(brushNode), Vector3(0, -1, 0));

    // Find the shared vertices of the two faces
    auto sharedVertices = findSharedVertices(*faceUp, *faceYDown);
    EXPECT_EQ(sharedVertices.size(), 2) << "There should be 2 shared 3D vertices between the two faces";

    for (const auto& pair : sharedVertices)
    {
        EXPECT_FALSE(math::isNear(pair.first->texcoord, pair.second->texcoord, 0.01))
            << "Texture coordinates are the same before pasting the texture projection";
    }

    // Position the camera top-down, similar to what an XY view is seeing
    render::View viewFaceUp(true);
    algorithm::constructCameraView(viewFaceUp, brushNode->localAABB(), { 0, 0, -1 }, { -90, 0, 0 });

    SelectionVolume testFaceUp(viewFaceUp);
    GlobalShaderClipboard().pickFromSelectionTest(testFaceUp);

    EXPECT_EQ(GlobalShaderClipboard().getSourceType(), selection::IShaderClipboard::SourceType::Face)
        << "Selection test failed to select the face";

    render::View viewFacingYUp(true);
    algorithm::constructCameraView(viewFacingYUp, brushNode->localAABB(), { 0, 1, 0 }, { 0, 90, 0 });

    ConstructSelectionTest(viewFacingYUp, selection::Rectangle::ConstructFromPoint({ 0, 0 }, { 0.1, 0.1 }));

    SelectionVolume testFacingYUp(viewFacingYUp);
    GlobalShaderClipboard().pasteShader(testFacingYUp, selection::PasteMode::Natural, false);

    for (const auto& pair : sharedVertices)
    {
        EXPECT_TRUE(math::isNear(pair.first->texcoord, pair.second->texcoord, 0.01))
            << "Texture coordinates should be the same after pasting the texture projection";
    }
}

TEST_F(TextureManipulationTest, NormaliseFace)
{
    std::string mapPath = "maps/simple_brushes.map";
    GlobalCommandSystem().executeCommand("OpenMap", mapPath);

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();

    // Find the brush that is centered at origin
    auto brushNode = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/4");
    EXPECT_TRUE(brushNode && brushNode->getNodeType() == scene::INode::Type::Brush) << "Couldn't locate the test brush";

    // Pick a few faces and run the algorithm against it, checking hardcoded results

    // Facing 0,1,0
    auto face = algorithm::findBrushFaceWithNormal(Node_getIBrush(brushNode), Vector3(0, 1, 0));

    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-760.0,-24.0, 208.0), Vector2(61.13169449567795, -0.5979519486427307)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-760.0,-24.0, 376.0), Vector2(61.13169449567795, -1.4489692151546478)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-992.0,-24.0, 376.0), Vector2(62.102946043014526, -1.4489692151546478)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-992.0,-24.0, 208.0), Vector2(62.102946043014526, -0.5979519486427307)));

    face->normaliseTexture();

    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-760.0, -24.0, 208.0), Vector2(0.13169449567795, 0.40204805135726929)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-760.0, -24.0, 376.0), Vector2(0.13169449567795, -0.4489692151546478)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-992.0, -24.0, 376.0), Vector2(1.102946043014526, -0.4489692151546478)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-992.0, -24.0, 208.0), Vector2(1.102946043014526, 0.40204805135726929)));

    // Facing 1,0,0
    face = algorithm::findBrushFaceWithNormal(Node_getIBrush(brushNode), Vector3(1, 0, 0));

    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-760.0, -280.0, 376.0), Vector2(-0.1557292342185974, 53.89497980847955)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-760.0, -24.0, 376.0),  Vector2(0.8797785639762878, 54.230962846428156)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-760.0, -24.0, 208.0),  Vector2(0.6975563578307629, 55.053220719099045)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-760.0, -280.0, 208.0), Vector2(-0.3379514403641224, 54.717237681150436)));

    face->normaliseTexture();

    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-760.0, -280.0, 376.0), Vector2(-0.1557292342185974, -0.10502019152045250)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-760.0, -24.0, 376.0), Vector2(0.8797785639762878, 0.230962846428156)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-760.0, -24.0, 208.0), Vector2(0.6975563578307629, 1.053220719099045)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-760.0, -280.0, 208.0), Vector2(-0.3379514403641224, 0.717237681150436)));

    // Facing 0,-1,0
    face = algorithm::findBrushFaceWithNormal(Node_getIBrush(brushNode), Vector3(0, -1, 0));

    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-992.0, -280.0, 376.0), Vector2(-15.5, -5.875)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-760.0, -280.0, 376.0), Vector2(-11.875, -5.875)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-760.0, -280.0, 208.0), Vector2(-11.875, -3.25)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-992.0, -280.0, 208.0), Vector2(-15.5, -3.25)));

    face->normaliseTexture();

    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-992.0,-280.0,376.0), Vector2(-2.5, -1.875)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-760.0,-280.0,376.0), Vector2(1.125, -1.875)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-760.0,-280.0,208.0), Vector2(1.125, 0.75)));
    EXPECT_TRUE(algorithm::faceHasVertex(face, Vector3(-992.0,-280.0,208.0), Vector2(-2.5, 0.75)));
}

TEST_F(TextureManipulationTest, NormalisePatch)
{
    std::string mapPath = "maps/simple_brushes.map";
    GlobalCommandSystem().executeCommand("OpenMap", mapPath);

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto patchNode = algorithm::findFirstPatchWithMaterial(worldspawn, "textures/numbers/4");
    auto patch = Node_getIPatch(patchNode);

    EXPECT_TRUE(math::isNear(patch->ctrlAt(0, 0).texcoord, { 45.2263, 28.4018}, 0.01));
    EXPECT_TRUE(math::isNear(patch->ctrlAt(1, 0).texcoord, { 43.855, 25.9462}, 0.01));
    EXPECT_TRUE(math::isNear(patch->ctrlAt(2, 0).texcoord, { 42.4838, 23.4907}, 0.01));
    EXPECT_TRUE(math::isNear(patch->ctrlAt(0, 1).texcoord, { 46.8088, 27.5181}, 0.01));
    EXPECT_TRUE(math::isNear(patch->ctrlAt(1, 1).texcoord, { 45.4375, 25.0625}, 0.01));
    EXPECT_TRUE(math::isNear(patch->ctrlAt(2, 1).texcoord, { 44.0662, 22.6069}, 0.01));
    EXPECT_TRUE(math::isNear(patch->ctrlAt(0, 2).texcoord, { 48.3912, 26.6343}, 0.01));
    EXPECT_TRUE(math::isNear(patch->ctrlAt(1, 2).texcoord, { 47.02, 24.1788}, 0.01));
    EXPECT_TRUE(math::isNear(patch->ctrlAt(2, 2).texcoord, { 45.6487, 21.7232}, 0.01));

    patch->normaliseTexture();

    EXPECT_TRUE(math::isNear(patch->ctrlAt(0, 0).texcoord, { 0.226303, 3.40177 }, 0.01));
    EXPECT_TRUE(math::isNear(patch->ctrlAt(1, 0).texcoord, { -1.14497, 0.946209 }, 0.01));
    EXPECT_TRUE(math::isNear(patch->ctrlAt(2, 0).texcoord, { -2.51625, -1.50935 }, 0.01));
    EXPECT_TRUE(math::isNear(patch->ctrlAt(0, 1).texcoord, { 1.80877, 2.51806 }, 0.01));
    EXPECT_TRUE(math::isNear(patch->ctrlAt(1, 1).texcoord, { 0.4375, 0.0625 }, 0.01));
    EXPECT_TRUE(math::isNear(patch->ctrlAt(2, 1).texcoord, { -0.933773, -2.39306 }, 0.01));
    EXPECT_TRUE(math::isNear(patch->ctrlAt(0, 2).texcoord, { 3.39125, 1.63435 }, 0.01));
    EXPECT_TRUE(math::isNear(patch->ctrlAt(1, 2).texcoord, { 2.01997, -0.821211 }, 0.01));
    EXPECT_TRUE(math::isNear(patch->ctrlAt(2, 2).texcoord, { 0.648697, -3.27677 }, 0.01));
}

namespace
{

void performTextureLockBrushTransformationTest(const std::function<void(ITransformablePtr)>& doTransform)
{
    registry::setValue(RKEY_ENABLE_TEXTURE_LOCK, true);

    std::string mapPath = "maps/simple_brushes.map";
    GlobalCommandSystem().executeCommand("OpenMap", mapPath);

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brushNode = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/1");
    auto brush = Node_getIBrush(brushNode);

    const auto& face = brush->getFace(0);
    std::vector<Vector2> oldTexCoords;
    for (const auto& vertex : face.getWinding())
    {
        oldTexCoords.push_back(vertex.texcoord);
    }

    auto transformable = scene::node_cast<ITransformable>(brushNode);

    if (transformable)
    {
        doTransform(transformable);
        transformable->freezeTransform();
    }

    // We need the texture coords to be up to date
    brush->evaluateBRep();

    auto old = oldTexCoords.begin();
    for (const auto& vertex : face.getWinding())
    {
        EXPECT_TRUE(math::isNear(*(old++), vertex.texcoord, 0.01)) << "Texture coord has been changed by transform";
    }
}

}

// Move a brush with texture lock enabled
TEST_F(TextureManipulationTest, MoveTextureLocked)
{
    performTextureLockBrushTransformationTest([&](const ITransformablePtr& transformable)
    {
        transformable->setType(TRANSFORM_PRIMITIVE);
        transformable->setTranslation(Vector3(-45.1, 66.3, 10.7));
    });
}

TEST_F(TextureManipulationTest, RotateTextureLocked)
{
    performTextureLockBrushTransformationTest([&](const ITransformablePtr& transformable)
    {
        transformable->setType(TRANSFORM_PRIMITIVE);
        transformable->setRotation(Quaternion::createForEulerXYZDegrees({ 5, 35, 75 }));
    });
}

namespace
{

inline scene::INodePtr create512CubeTextured1x1(const std::string& material)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto test1024x512Node = algorithm::createCuboidBrush(worldspawn, AABB({ 30, 40, -60}, { 256, 256, 256 }), material);
    auto test1024x512 = Node_getIBrush(test1024x512Node);

    algorithm::foreachFace(*Node_getIBrush(test1024x512Node), [](IFace& face) { face.fitTexture(1, 1); });

    return test1024x512Node;
}

}

TEST_F(TextureManipulationTest, FaceGetTexelScale)
{
    auto brushNode = create512CubeTextured1x1("textures/a_1024x512");

    // Get the texture dimensions
    auto editorImage = GlobalMaterialManager().getMaterial("textures/a_1024x512")->getEditorImage();
    auto textureWidth = editorImage->getWidth();
    auto textureHeight = editorImage->getHeight();

    // Get a face and check the texture bounds
    auto& face = Node_getIBrush(brushNode)->getFace(0);
    auto uvBounds = algorithm::getTextureSpaceBounds(face);

    // Check that the face is showing a 1x1 tile
    EXPECT_NEAR(uvBounds.extents.x(), 0.5, 0.01) << "Texture not fit to face (X)";
    EXPECT_NEAR(uvBounds.extents.y(), 0.5, 0.01) << "Texture not fit to face (Y)";

    auto texelScale = face.getTexelScale();

    // A 1024 texture width fit to a 512px wide face: texel scale = 2
    // A 512 texture height fit to a 512px wide face: texel scale = 1
    auto expectedTexelScaleX = textureWidth / 512.0;
    auto expectedTexelScaleY = textureHeight / 512.0;

    EXPECT_NEAR(texelScale.x(), expectedTexelScaleX, 0.01) << "Texel scale X is off";
    EXPECT_NEAR(texelScale.y(), expectedTexelScaleY, 0.01) << "Texel scale Y is off";
}

TEST_F(TextureManipulationTest, FaceGetTextureAspectRatio)
{
    auto brushNode = create512CubeTextured1x1("textures/a_1024x512");

    // Get the texture dimensions
    auto editorImage = GlobalMaterialManager().getMaterial("textures/a_1024x512")->getEditorImage();
    auto textureWidth = editorImage->getWidth();
    auto textureHeight = editorImage->getHeight();

    // Get a face and check the texture bounds
    auto& face = Node_getIBrush(brushNode)->getFace(0);

    EXPECT_NEAR(face.getTextureAspectRatio(), static_cast<float>(textureWidth) / textureHeight, 0.01)
        << "Wrong texture aspect ratio reported";

    brushNode = create512CubeTextured1x1("textures/numbers/0");
    auto& anotherFace = Node_getIBrush(brushNode)->getFace(0);
    EXPECT_NEAR(anotherFace.getTextureAspectRatio(), 1.0, 0.001)
        << "Number texture aspect ratio should be 1.0";
}

TEST_F(TextureManipulationTest, PatchGetTextureAspectRatio)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto patchNode = algorithm::createPatchFromBounds(worldspawn, AABB(Vector3(4, 50, 60), Vector3(64, 128, 256)), "textures/a_1024x512");
    auto patch = Node_getIPatch(patchNode);

    // Get the texture dimensions
    auto editorImage = GlobalMaterialManager().getMaterial(patch->getShader())->getEditorImage();
    auto textureWidth = editorImage->getWidth();
    auto textureHeight = editorImage->getHeight();

    EXPECT_NEAR(patch->getTextureAspectRatio(), static_cast<float>(textureWidth) / textureHeight, 0.01)
        << "Wrong texture aspect ratio reported";

    patchNode = algorithm::createPatchFromBounds(worldspawn, AABB(Vector3(4, 50, 60), Vector3(64, 128, 256)), "textures/numbers/0");
    patch = Node_getIPatch(patchNode);

    EXPECT_NEAR(patch->getTextureAspectRatio(), 1.0, 0.001) << "Number texture aspect ratio should be 1.0";
}

TEST_F(TextureManipulationTest, FaceTextureChangePreservesTexelScale)
{
    auto largeTexture = "textures/a_1024x512";
    auto smallTexture = "textures/numbers/0";

    // Get a face and check the texture bounds (brush setup is checked in FaceGetTexelScale)
    auto brushNode = create512CubeTextured1x1(largeTexture);
    auto& face = Node_getIBrush(brushNode)->getFace(0);

    // Get the texture dimensions
    auto editorImage = GlobalMaterialManager().getMaterial(largeTexture)->getEditorImage();
    auto textureWidth = editorImage->getWidth();
    auto textureHeight = editorImage->getHeight();

    auto texelScaleBefore = face.getTexelScale();

    // Get the texture dimensions
    auto numberImage = GlobalMaterialManager().getMaterial(smallTexture)->getEditorImage();
    auto numberImageWidth = numberImage->getWidth();
    auto numberImageHeight = numberImage->getHeight();

    // We need the textures to be different
    EXPECT_NE(numberImageWidth, textureWidth);
    EXPECT_NE(numberImageHeight, textureHeight);

    face.setShader(smallTexture);

    auto texelScaleAfter = face.getTexelScale();

    EXPECT_NEAR(texelScaleBefore.x(), texelScaleAfter.x(), 0.01) << "Texel scale X changed after applying new material";
    EXPECT_NEAR(texelScaleBefore.y(), texelScaleAfter.y(), 0.01) << "Texel scale Y changed after applying new material";
}

TEST_F(TextureManipulationTest, FaceRotationPreservesTexelScale)
{
    auto material = "textures/a_1024x512";
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto test1024x512Node = algorithm::createCuboidBrush(worldspawn, AABB({ 128, 32, 16 }, { 128, 32, 16 }), material);
    algorithm::foreachFace(*Node_getIBrush(test1024x512Node), [](IFace& face) { face.fitTexture(1, 1); });

    auto& face = *algorithm::findBrushFaceWithNormal(Node_getIBrush(test1024x512Node), { 0, 0, 1 });

    auto texelScaleBefore = face.getTexelScale();

    face.rotateTexdef(45); // degrees
    face.rotateTexdef(45);

    auto texelScaleAfter = face.getTexelScale();

    EXPECT_NEAR(texelScaleBefore.x(), texelScaleAfter.x(), 0.01) << "Texel scale X changed after rotation by 90 degrees";
    EXPECT_NEAR(texelScaleBefore.y(), texelScaleAfter.y(), 0.01) << "Texel scale Y changed after rotation by 90 degrees";
}

TEST_F(TextureManipulationTest, PatchRotationPreservesTexelScale)
{
    auto material = "textures/a_1024x512";
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto patchNode = algorithm::createPatchFromBounds(worldspawn, AABB(Vector3(4, 50, 60), Vector3(64, 128, 256)), "textures/a_1024x512");
    auto patch = Node_getIPatch(patchNode);
    patch->fitTexture(1, 1);

    EXPECT_NEAR(patch->getTextureAspectRatio(), 2, 0.01) << "This test needs a 2:1 texture aspect";

    // The texture bounds should be 1x1, since the texture is fitted
    auto boundsBefore = algorithm::getTextureSpaceBounds(*patch);

    EXPECT_NEAR(boundsBefore.extents.x() * 2, 1, 0.01) << "Patch UV bounds should be 1x1 before rotating";
    EXPECT_NEAR(boundsBefore.extents.y() * 2, 1, 0.01) << "Patch UV bounds should be 1x1 before rotating";

    patch->rotateTexture(45); // degrees
    patch->rotateTexture(45);

    auto boundsAfter = algorithm::getTextureSpaceBounds(*patch);

    EXPECT_NEAR(boundsAfter.extents.x() * 2, 0.5, 0.01) << "Patch UV bounds should be 0.5x2 after rotating";
    EXPECT_NEAR(boundsAfter.extents.y() * 2, 2, 0.01) << "Patch UV bounds should be 0.5x2 after rotating";
}

TEST_F(TextureManipulationTest, FaceGetShiftScaleRotation)
{
    // These materials have an editor image with 2:1 aspect ratio
    auto materialA = "textures/a_1024x512";
    auto materialB = "textures/b_1024x512";

    std::string mapPath = "maps/simple_brushes.map";
    GlobalCommandSystem().executeCommand("OpenMap", mapPath);

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();

    // Brush A
    auto brushNode = algorithm::findFirstBrushWithMaterial(worldspawn, materialA);
    auto brush = Node_getIBrush(brushNode);

    auto& faceA = *algorithm::findBrushFaceWithNormal(brush, { 0, 0, 1 });

    auto ssr = faceA.getShiftScaleRotation();

    EXPECT_NEAR(ssr.scale[0], 0.25, 0.01) << "Brush A: Horizontal Scale is off";
    EXPECT_NEAR(ssr.scale[1], 0.25, 0.01) << "Brush A: Vertical Scale is off";
    EXPECT_NEAR(ssr.rotate, 90, 0.01) << "Brush A: Rotation Value is off";

    // Brush B (is rotated, but should show the same scale values
    brushNode = algorithm::findFirstBrushWithMaterial(worldspawn, materialB);
    brush = Node_getIBrush(brushNode);

    auto& faceB = *algorithm::findBrushFaceWithNormal(brush, { 0, 0, 1 });

    ssr = faceB.getShiftScaleRotation();

    EXPECT_NEAR(ssr.scale[0], 0.25, 0.01) << "Brush B: Horizontal Scale is off";
    EXPECT_NEAR(ssr.scale[1], 0.25, 0.01) << "Brush B: Vertical Scale is off";
    EXPECT_NEAR(ssr.rotate, 75, 0.01) << "Brush B: Rotation Value is off";
}

// #5846: Rotating a func_static brush 90 degress messes up the face textures
TEST_F(TextureManipulationTest, RotateFuncStaticBrush90)
{
    std::string mapPath = "maps/rotate_with_texlock.map";
    GlobalCommandSystem().executeCommand("OpenMap", mapPath);

    auto funcStatic = algorithm::getEntityByName(GlobalMapModule().getRoot(), "func_static_1");

    auto brushNode = algorithm::getNthChild(funcStatic, 0);
    auto brush = Node_getIBrush(brushNode);

    auto& faceBefore = *algorithm::findBrushFaceWithNormal(brush, { 1, 0, 0 });
    
    // Record the texture coordinates of this face
    std::vector<WindingVertex> oldVertices;
    for (const auto& vertex : faceBefore.getWinding())
    {
        oldVertices.push_back(vertex);
    }

    // Select and rotate that func_static
    Node_setSelected(funcStatic, true);
    GlobalCommandSystem().executeCommand("RotateSelectionZ");

    // Trigger bounds evalation which will make the brush evaluate the plane intersections
    // Same happens when rendering the scene after the brush evaluation
    funcStatic->worldAABB();

    auto& faceAfter = *algorithm::findBrushFaceWithNormal(brush, { 1, 0, 0 });

    std::optional<Vector2> distance;
    auto old = oldVertices.begin();
    for (const auto& vertex : faceAfter.getWinding())
    {
        // Assume the 3D coordinates have changed
        EXPECT_FALSE(math::isNear(vertex.vertex, old->vertex, 0.01));

        // The texture coordinates should remain equivalent (due to texture lock)
        // The absolute coordinates in UV space might be off by some integer number
        // We expect the distance to the previous coordinates to be the same
        if (distance.has_value())
        {
            EXPECT_TRUE(math::isNear(distance.value(), vertex.texcoord - old->texcoord, 0.01));
        }
        else
        {
            distance = vertex.texcoord - old->texcoord;
        }

        ++old;
    }
}

}
