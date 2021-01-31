#include "RadiantTest.h"

#include <algorithm>
#include "ishaders.h"
#include "imap.h"
#include "ifilter.h"
#include "ilightnode.h"
#include "ibrush.h"
#include "ipatch.h"
#include "ientity.h"
#include "ishaders.h"
#include "ieclass.h"
#include "algorithm/Scene.h"
#include "scenelib.h"
#include "selectionlib.h"
#include "string/convert.h"
#include "render/View.h"
#include "render/CameraView.h"
#include "selection/SelectionVolume.h"
#include "Rectangle.h"

namespace test
{

using SelectionTest = RadiantTest;

TEST_F(SelectionTest, ApplyShadersToForcedVisibleObjects)
{
    loadMap("primitives_with_clip_material.map");

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/common/clip");
    auto patch = algorithm::findFirstPatchWithMaterial(worldspawn, "textures/common/clip");

    // Filter out the objects using the clip material
    GlobalFilterSystem().setFilterState("Clip Textures", true);

    // Glitch: Since this brush is completely filtered out, this won't be called by the filter system
    Node_getIBrush(brush)->updateFaceVisibility();

    EXPECT_FALSE(brush->visible());
    EXPECT_FALSE(patch->visible());

    Node_setSelected(brush, true);
    Node_setSelected(patch, true);

    EXPECT_TRUE(brush->visible());
    EXPECT_TRUE(patch->visible());

    // Apply the caulk material to the selection
    selection::applyShaderToSelection("textures/common/caulk");

    auto brushNode = std::dynamic_pointer_cast<IBrushNode>(brush);
    auto patchNode = std::dynamic_pointer_cast<IPatchNode>(patch);

    EXPECT_TRUE(brushNode->getIBrush().hasShader("textures/common/caulk"));
    EXPECT_EQ(patchNode->getPatch().getShader(), ("textures/common/caulk"));
}

// #5443: Size display of lights doesn't change when modifying "light_radius"
TEST_F(SelectionTest, LightBoundsChangedAfterRadiusChange)
{
    auto eclass = GlobalEntityClassManager().findOrInsert("light", false);
    auto entityNode = GlobalEntityModule().createEntity(eclass);

    EXPECT_TRUE(Node_getLightNode(entityNode));
    auto* entity = Node_getEntity(entityNode);
    entity->setKeyValue("light_radius", "300 200 100");
    GlobalMapModule().getRoot()->addChildNode(entityNode);

    Node_setSelected(entityNode, true);

    auto defaultBounds = string::convert<Vector3>(entity->getKeyValue("light_radius"));

    EXPECT_TRUE(GlobalSelectionSystem().getWorkZone().bounds.getExtents().isEqual(defaultBounds, 0.01));

    // Modify just the light_radius spawnarg
    entity->setKeyValue("light_radius", "30 20 10");

    // The work zone should have adapted itself to the new bounds
    // assuming that the LightNode recalculates its AABB
    auto changedBounds = string::convert<Vector3>(entity->getKeyValue("light_radius"));
    EXPECT_TRUE(GlobalSelectionSystem().getWorkZone().bounds.getExtents().isEqual(changedBounds, 0.01));
}

// #5484: Projected lights don't rotate around their origin anymore
TEST_F(SelectionTest, SelectionBoundsOfProjectedLights)
{
    auto eclass = GlobalEntityClassManager().findOrInsert("light", false);
    auto entityNode = GlobalEntityModule().createEntity(eclass);

    EXPECT_TRUE(Node_getLightNode(entityNode));
    auto* entity = Node_getEntity(entityNode);

    entity->setKeyValue("origin", "0 0 0");
    entity->setKeyValue("light_target", "0 0 -256");
    entity->setKeyValue("light_right", "128 0 0");
    entity->setKeyValue("light_up", "0 128 0");
    entity->setKeyValue("light_start", "");
    entity->setKeyValue("light_end", "");

    GlobalMapModule().getRoot()->addChildNode(entityNode);

    Node_setSelected(entityNode, true);

    // The center of the selection AABB needs to be at its origin
    EXPECT_EQ(Node_getLightNode(entityNode)->getSelectAABB().getOrigin(), Vector3(0,0,0));
}

constexpr std::size_t DeviceWidth = 640;
constexpr std::size_t DeviceHeight = 640;

class ViewSelectionTest :
    public SelectionTest
{
protected:
    virtual render::View createView() = 0;

    virtual void constructView(render::View& view, const AABB& objectAABB) = 0;

    void performViewSelectionTest(render::View& view, const scene::INodePtr& node, bool expectNodeIsSelectable)
    {
        // Selection Point
        auto rectangle = selection::Rectangle::ConstructFromPoint(Vector2(0, 0), Vector2(8.0 / DeviceWidth, 8.0 / DeviceHeight));
        ConstructSelectionTest(view, rectangle);

        EXPECT_FALSE(Node_isSelected(node));

        SelectionVolume test(view);
        GlobalSelectionSystem().selectPoint(test, SelectionSystem::eToggle, false);

        EXPECT_EQ(Node_isSelected(node), expectNodeIsSelectable);

        // De-select to be sure
        Node_setSelected(node, false);
    }

    void performSelectionTest(const scene::INodePtr& node, bool expectNodeIsSelectable)
    {
        // Move the orthoview exactly to the center of this object
        render::View view = createView();
        constructView(view, node->worldAABB());

        performViewSelectionTest(view, node, expectNodeIsSelectable);
    }

    void performBrushSelectionTest(const std::string& materialName, bool expectNodeIsSelectable)
    {
        // Filter caulk faces
        auto material = GlobalMaterialManager().getMaterialForName("textures/common/caulk");
        material->setVisible(false);

        auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
        auto brush = algorithm::findFirstBrushWithMaterial(worldspawn, materialName);
        EXPECT_TRUE(brush);

        Node_getIBrush(brush)->updateFaceVisibility();

        performSelectionTest(brush, expectNodeIsSelectable);
    }

    void performPatchSelectionTest(const std::string& materialName, bool expectNodeIsSelectable)
    {
        auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
        auto patch = algorithm::findFirstPatchWithMaterial(worldspawn, materialName);
        EXPECT_TRUE(patch);

        performSelectionTest(patch, expectNodeIsSelectable);
    }

    void performModelSelectionTest(const std::string& entityName, bool expectNodeIsSelectable)
    {
        auto entity = algorithm::getEntityByName(GlobalMapModule().getRoot(), entityName);
        EXPECT_TRUE(entity);

        performSelectionTest(entity, expectNodeIsSelectable);
    }
};

class OrthoViewSelectionTest :
    public ViewSelectionTest
{
protected:
    virtual render::View createView() override
    {
        return render::View();
    }

    virtual void constructView(render::View& view, const AABB& objectAABB) override
    {
        // Move the orthoview exactly to the center of this object
        double scale = 1.0;

        Matrix4 projection;

        projection[0] = 1.0 / static_cast<double>(DeviceWidth / 2);
        projection[5] = 1.0 / static_cast<double>(DeviceHeight / 2);
        projection[10] = 1.0 / (32768 * scale);

        projection[12] = 0.0;
        projection[13] = 0.0;
        projection[14] = -1.0;

        projection[1] = projection[2] = projection[3] =
            projection[4] = projection[6] = projection[7] =
            projection[8] = projection[9] = projection[11] = 0.0;

        projection[15] = 1.0f;

        // Modelview
        Matrix4 modelView;

        // Translate the view to the center of the brush
        modelView[12] = -objectAABB.getOrigin().x() * scale;
        modelView[13] = -objectAABB.getOrigin().y() * scale;
        modelView[14] = 32768 * scale;

        // axis base
        modelView[0] = scale;
        modelView[1] = 0;
        modelView[2] = 0;

        modelView[4] = 0;
        modelView[5] = scale;
        modelView[6] = 0;

        modelView[8] = 0;
        modelView[9] = 0;
        modelView[10] = -scale;

        modelView[3] = modelView[7] = modelView[11] = 0;
        modelView[15] = 1;

        view.construct(projection, modelView, DeviceWidth, DeviceHeight);
    }
};

class CameraViewSelectionTest :
    public ViewSelectionTest
{
protected:
    virtual render::View createView() override
    {
        return render::View(true);
    }

    virtual void constructView(render::View& view, const AABB& objectAABB) override
    {
        // Position the camera top-down, similar to what an XY view is seeing
        auto objectHeight = std::max(objectAABB.getExtents().z(), 20.0); // use a minimum height
        Vector3 origin = objectAABB.getOrigin() + Vector3(0, 0, objectHeight * 3);
        Vector3 angles(-90, 0, 0);

        auto farClip = 32768.0f;
        Matrix4 projection = camera::calculateProjectionMatrix(farClip / 4096.0f, farClip, 75.0f, DeviceWidth, DeviceHeight);
        Matrix4 modelview = camera::calculateModelViewMatrix(origin, angles);

        view.construct(projection, modelview, DeviceWidth, DeviceHeight);
    }
};

TEST_F(OrthoViewSelectionTest, BrushFacingTowardsViewIsSelectable)
{
    loadMap("selection_test.map");

    performBrushSelectionTest("textures/numbers/1", true);
}

TEST_F(CameraViewSelectionTest, BrushFacingTowardsViewIsSelectable)
{
    loadMap("selection_test.map");

    performBrushSelectionTest("textures/numbers/1", true);
}

// #5444: Brushes faces facing away were unselectable in orthoview
TEST_F(OrthoViewSelectionTest, BrushFacingAwayFromViewIsSelectable)
{
    loadMap("selection_test.map");

    performBrushSelectionTest("textures/numbers/2", true); // should be selectable
}

TEST_F(CameraViewSelectionTest, BrushFacingAwayFromViewIsNotSelectable)
{
    loadMap("selection_test.map");

    performBrushSelectionTest("textures/numbers/2", false); // not selectable
}

TEST_F(OrthoViewSelectionTest, PatchFacingTowardsViewIsSelectable)
{
    loadMap("selection_test.map");

    performPatchSelectionTest("textures/numbers/1", true);
}

TEST_F(CameraViewSelectionTest, PatchFacingTowardsViewIsSelectable)
{
    loadMap("selection_test.map");

    performPatchSelectionTest("textures/numbers/1", true);
}

TEST_F(OrthoViewSelectionTest, PatchFacingAwayFromViewIsSelectable)
{
    loadMap("selection_test.map");

    performPatchSelectionTest("textures/numbers/2", true);
}

TEST_F(CameraViewSelectionTest, PatchFacingAwayFromViewIsNotSelectable)
{
    loadMap("selection_test.map");

    performPatchSelectionTest("textures/numbers/2", false);
}

TEST_F(OrthoViewSelectionTest, TwosidedModelFacingAwayIsSelectable)
{
    loadMap("twosided_ivy.mapx");

    performModelSelectionTest("ivy_facing_down", true);
}

TEST_F(CameraViewSelectionTest, TwosidedModelFacingAwayIsSelectable)
{
    loadMap("twosided_ivy.mapx");

    performModelSelectionTest("ivy_facing_down", true);
}

TEST_F(OrthoViewSelectionTest, TwosidedModelFacingUpIsSelectable)
{
    loadMap("twosided_ivy.mapx");

    performModelSelectionTest("ivy_facing_up", true);
}

TEST_F(CameraViewSelectionTest, TwosidedModelFacingUpIsSelectable)
{
    loadMap("twosided_ivy.mapx");

    performModelSelectionTest("ivy_facing_up", true);
}

}
