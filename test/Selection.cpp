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
#include "algorithm/Primitives.h"
#include "scenelib.h"
#include "selectionlib.h"
#include "string/convert.h"
#include "render/View.h"
#include "render/CameraView.h"
#include "selection/SelectionVolume.h"
#include "Rectangle.h"
#include "registry/registry.h"
#include "algorithm/View.h"

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

    EXPECT_TRUE(math::isNear(GlobalSelectionSystem().getWorkZone().bounds.getExtents(), defaultBounds, 0.01));

    // Modify just the light_radius spawnarg
    entity->setKeyValue("light_radius", "30 20 10");

    // The work zone should have adapted itself to the new bounds
    // assuming that the LightNode recalculates its AABB
    auto changedBounds = string::convert<Vector3>(entity->getKeyValue("light_radius"));
    EXPECT_TRUE(math::isNear(GlobalSelectionSystem().getWorkZone().bounds.getExtents(), changedBounds, 0.01));
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

// #4846: Rotation widget does not re-center on selected object
TEST_F(SelectionTest, PivotIsResetAfterCancelingOperation)
{
    loadMap("selection_test.map");

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/1");

    GlobalSelectionSystem().setActiveManipulator(selection::Manipulator::Translate);

    // Select the node, the pivot should now be at the node center
    Node_setSelected(brush, true);

    Vector3 originalBrushPosition = brush->worldAABB().getOrigin();

    Matrix4 originalPivot = GlobalSelectionSystem().getPivot2World();
    EXPECT_EQ(originalPivot.translation(), originalBrushPosition);

    const auto& activeManipulator = GlobalSelectionSystem().getActiveManipulator();

    // Construct an orthoview to test-select the manipulator
    render::View view(false);
    algorithm::constructCenteredOrthoview(view, originalBrushPosition);

    SelectionVolume test = algorithm::constructOrthoviewSelectionTest(view);
    activeManipulator->testSelect(test, originalPivot);

    EXPECT_TRUE(activeManipulator->isSelected());

    GlobalSelectionSystem().onManipulationStart();
    activeManipulator->getActiveComponent()->beginTransformation(originalPivot, view, Vector2(0, 0));

    // Transform by dragging stuff around
    activeManipulator->getActiveComponent()->transform(originalPivot, view, Vector2(0.5, 0.5), false);
    GlobalSelectionSystem().onManipulationChanged();

    // The brush should have been moved
    EXPECT_NE(brush->worldAABB().getOrigin(), originalBrushPosition);
    // Pivot should have been moved
    EXPECT_NE(GlobalSelectionSystem().getPivot2World().translation(), originalBrushPosition);

    // Now cancel the operation
    GlobalSelectionSystem().onManipulationCancelled();

    // Brush should be back at its original position
    EXPECT_EQ(brush->worldAABB().getOrigin(), originalBrushPosition);
    // And as of #4846 the pivot should be back as well
    EXPECT_EQ(GlobalSelectionSystem().getPivot2World().translation(), originalBrushPosition);
}

// #5460: Workzone not recalculated after selection change if XY view "Show Size Info" setting is off
TEST_F(SelectionTest, WorkzoneIsRecalculatedAfterSelectionChange)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();

    AABB tallBounds(Vector3(0, 0, 0), Vector3(64, 256, 128));
    AABB smallBounds(Vector3(300, 300, 300), Vector3(64, 32, 64));

    auto tallBrush = algorithm::createCuboidBrush(worldspawn, tallBounds);
    auto smallBrush = algorithm::createCuboidBrush(worldspawn, smallBounds);

    EXPECT_EQ(tallBrush->worldAABB().getOrigin(), tallBounds.getOrigin());
    EXPECT_EQ(tallBrush->worldAABB().getExtents(), tallBounds.getExtents());
    EXPECT_EQ(smallBrush->worldAABB().getOrigin(), smallBounds.getOrigin());
    EXPECT_EQ(smallBrush->worldAABB().getExtents(), smallBounds.getExtents());

    GlobalSelectionSystem().setSelectedAll(false);

    render::View orthoView(false);

    // Construct an orthoview to test-select the tall brush
    algorithm::constructCenteredOrthoview(orthoView, tallBrush->worldAABB().getOrigin());
    auto tallBrushTest = algorithm::constructOrthoviewSelectionTest(orthoView);

    // Select and de-select first brush
    GlobalSelectionSystem().selectPoint(tallBrushTest, SelectionSystem::eToggle, false);
    EXPECT_TRUE(Node_isSelected(tallBrush));

    // Workzone should match the size of the tall brush
    EXPECT_EQ(GlobalSelectionSystem().getWorkZone().bounds, tallBounds);

    // De-select the tall brush
    GlobalSelectionSystem().selectPoint(tallBrushTest, SelectionSystem::eToggle, false);
    EXPECT_FALSE(Node_isSelected(tallBrush));

    // Workzone should still match the size of the tall brush
    EXPECT_EQ(GlobalSelectionSystem().getWorkZone().bounds, tallBounds);

    // Construct an orthoview to test-select the smaller brush
    algorithm::constructCenteredOrthoview(orthoView, smallBrush->worldAABB().getOrigin());
    auto smallBrushTest = algorithm::constructOrthoviewSelectionTest(orthoView);

    // Select and de-select second brush (no getWorkZone() call in between)
    GlobalSelectionSystem().selectPoint(smallBrushTest, SelectionSystem::eToggle, false);
    EXPECT_TRUE(Node_isSelected(smallBrush));
    GlobalSelectionSystem().selectPoint(smallBrushTest, SelectionSystem::eToggle, false);
    EXPECT_FALSE(Node_isSelected(smallBrush));

    // Workzone should match the size of the small brush now
    EXPECT_EQ(GlobalSelectionSystem().getWorkZone().bounds, smallBounds);
}

class ViewSelectionTest :
    public SelectionTest
{
protected:
    virtual render::View createView() = 0;

    virtual void constructView(render::View& view, const AABB& objectAABB) = 0;

    void performViewSelectionTest(render::View& view, const scene::INodePtr& node, bool expectNodeIsSelectable)
    {
        // Selection Point
        auto rectangle = selection::Rectangle::ConstructFromPoint(Vector2(0, 0), Vector2(8.0 / algorithm::DeviceWidth, 8.0 / algorithm::DeviceHeight));
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
        auto material = GlobalMaterialManager().getMaterial("textures/common/caulk");
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
        algorithm::constructCenteredOrthoview(view, objectAABB.getOrigin());
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
        Matrix4 projection = camera::calculateProjectionMatrix(farClip / 4096.0f, farClip, 75.0f, algorithm::DeviceWidth, algorithm::DeviceHeight);
        Matrix4 modelview = camera::calculateModelViewMatrix(origin, angles);

        view.construct(projection, modelview, algorithm::DeviceWidth, algorithm::DeviceHeight);
    }
};

// --------- Brush with one-sided material -----

TEST_F(OrthoViewSelectionTest, OnesidedBrushFacingTowardsViewIsSelectable)
{
    loadMap("selection_test.map");

    performBrushSelectionTest("textures/numbers/1", true);
}

TEST_F(CameraViewSelectionTest, OnesidedBrushFacingTowardsViewIsSelectable)
{
    loadMap("selection_test.map");

    performBrushSelectionTest("textures/numbers/1", true);
}

// #5444: Brushes faces facing away were unselectable in orthoview
TEST_F(OrthoViewSelectionTest, OnesidedBrushFacingAwayFromViewIsSelectable)
{
    loadMap("selection_test.map");

    performBrushSelectionTest("textures/numbers/2", true); // selectable in ortho
}

TEST_F(CameraViewSelectionTest, OnesidedBrushFacingAwayFromViewIsNotSelectable)
{
    loadMap("selection_test.map");

    performBrushSelectionTest("textures/numbers/2", false); // not selectable in camera
}

// --------- Brush face with two-sided material -----

TEST_F(OrthoViewSelectionTest, TwosidedBrushFacingTowardsViewIsSelectable)
{
    loadMap("selection_test.map");

    performBrushSelectionTest("textures/darkmod/decals/vegetation/ivy_mixed_pieces", true);
}

TEST_F(CameraViewSelectionTest, TwosidedBrushFacingTowardsViewIsSelectable)
{
    loadMap("selection_test.map");

    performBrushSelectionTest("textures/darkmod/decals/vegetation/ivy_mixed_pieces", true);
}

TEST_F(OrthoViewSelectionTest, TwosidedBrushFacingAwayFromViewIsSelectable)
{
    loadMap("selection_test.map");

    performBrushSelectionTest("textures/darkmod/decals/vegetation/ivy_mixed_pieces2", true); // selectable in ortho
}

TEST_F(CameraViewSelectionTest, TwosidedBrushFacingAwayFromViewIsNotSelectable)
{
    loadMap("selection_test.map");

    performBrushSelectionTest("textures/darkmod/decals/vegetation/ivy_mixed_pieces2", false); // not selectable in camera, face is not visible
}

// --------- Patch with one-sided material -----

TEST_F(OrthoViewSelectionTest, OnesidedPatchFacingTowardsViewIsSelectable)
{
    loadMap("selection_test.map");

    performPatchSelectionTest("textures/numbers/1", true);
}

TEST_F(CameraViewSelectionTest, OnesidedPatchFacingTowardsViewIsSelectable)
{
    loadMap("selection_test.map");

    performPatchSelectionTest("textures/numbers/1", true);
}

TEST_F(OrthoViewSelectionTest, OnesidedPatchFacingAwayFromViewIsSelectable)
{
    loadMap("selection_test.map");

    performPatchSelectionTest("textures/numbers/2", true);
}

TEST_F(CameraViewSelectionTest, OnesidedPatchFacingAwayFromViewIsNotSelectable)
{
    loadMap("selection_test.map");

    performPatchSelectionTest("textures/numbers/2", false);
}

// --------- Patch with two-sided material -----

TEST_F(OrthoViewSelectionTest, TwosidedPatchFacingTowardsViewIsSelectable)
{
    loadMap("selection_test.map");

    performPatchSelectionTest("textures/darkmod/decals/vegetation/ivy_mixed_pieces2", true);
}

TEST_F(CameraViewSelectionTest, TwosidedPatchFacingTowardsViewIsSelectable)
{
    loadMap("selection_test.map");

    performPatchSelectionTest("textures/darkmod/decals/vegetation/ivy_mixed_pieces2", true);
}

TEST_F(OrthoViewSelectionTest, TwosidedPatchFacingAwayFromViewIsSelectable)
{
    loadMap("selection_test.map");

    performPatchSelectionTest("textures/darkmod/decals/vegetation/ivy_mixed_pieces2", true); // selectable in ortho
}

TEST_F(CameraViewSelectionTest, TwosidedPatchFacingAwayFromViewIsSelectable)
{
    loadMap("selection_test.map");

    performPatchSelectionTest("textures/darkmod/decals/vegetation/ivy_mixed_pieces2", true); // selectable in camera
}

// --------- Model with one-sided material -----

TEST_F(OrthoViewSelectionTest, OnesidedModelFacingAwayIsSelectable)
{
    loadMap("twosided_ivy.mapx");

    performModelSelectionTest("onesided_ivy_facing_down", true);
}

TEST_F(CameraViewSelectionTest, OnesidedModelFacingAwayIsNotSelectable)
{
    loadMap("twosided_ivy.mapx");

    performModelSelectionTest("onesided_ivy_facing_down", false);
}

TEST_F(OrthoViewSelectionTest, OnesidedModelFacingUpIsSelectable)
{
    loadMap("twosided_ivy.mapx");

    performModelSelectionTest("onesided_ivy_facing_up", true);
}

TEST_F(CameraViewSelectionTest, OnesidedModelFacingUpIsSelectable)
{
    loadMap("twosided_ivy.mapx");

    performModelSelectionTest("onesided_ivy_facing_up", true);
}

// --------- Model with two-sided material -----

TEST_F(OrthoViewSelectionTest, TwosidedModelFacingAwayIsSelectable)
{
    loadMap("twosided_ivy.mapx");

    performModelSelectionTest("twosided_ivy_facing_down", true);
}

TEST_F(CameraViewSelectionTest, TwosidedModelFacingAwayIsSelectable)
{
    loadMap("twosided_ivy.mapx");

    performModelSelectionTest("twosided_ivy_facing_down", true);
}

TEST_F(OrthoViewSelectionTest, TwosidedModelFacingUpIsSelectable)
{
    loadMap("twosided_ivy.mapx");

    performModelSelectionTest("twosided_ivy_facing_up", true);
}

TEST_F(CameraViewSelectionTest, TwosidedModelFacingUpIsSelectable)
{
    loadMap("twosided_ivy.mapx");

    performModelSelectionTest("twosided_ivy_facing_up", true);
}

}
