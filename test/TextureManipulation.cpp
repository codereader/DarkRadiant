#include "RadiantTest.h"

#include "ishaderclipboard.h"
#include "algorithm/Scene.h"
#include "algorithm/Primitives.h"
#include "scenelib.h"
#include "math/Matrix3.h"
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

}
