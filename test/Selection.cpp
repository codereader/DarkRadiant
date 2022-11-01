#include "RadiantTest.h"

#include "ishaders.h"
#include "imap.h"
#include "ifilter.h"
#include "ilightnode.h"
#include "ibrush.h"
#include "ipatch.h"
#include "ientity.h"
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
#include "algorithm/View.h"
#include "algorithm/XmlUtils.h"

namespace test
{

using SelectionTest = RadiantTest;

TEST_F(SelectionTest, DefaultSelectionMode)
{
    EXPECT_EQ(GlobalSelectionSystem().getSelectionMode(), selection::SelectionMode::Primitive);
}

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

    GlobalSelectionSystem().setActiveManipulator(selection::IManipulator::Translate);

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
    GlobalSelectionSystem().selectPoint(tallBrushTest, selection::SelectionSystem::eToggle, false);
    EXPECT_TRUE(Node_isSelected(tallBrush));

    // Workzone should match the size of the tall brush
    EXPECT_EQ(GlobalSelectionSystem().getWorkZone().bounds, tallBounds);

    // De-select the tall brush
    GlobalSelectionSystem().selectPoint(tallBrushTest, selection::SelectionSystem::eToggle, false);
    EXPECT_FALSE(Node_isSelected(tallBrush));

    // Workzone should still match the size of the tall brush
    EXPECT_EQ(GlobalSelectionSystem().getWorkZone().bounds, tallBounds);

    // Construct an orthoview to test-select the smaller brush
    algorithm::constructCenteredOrthoview(orthoView, smallBrush->worldAABB().getOrigin());
    auto smallBrushTest = algorithm::constructOrthoviewSelectionTest(orthoView);

    // Select and de-select second brush (no getWorkZone() call in between)
    GlobalSelectionSystem().selectPoint(smallBrushTest, selection::SelectionSystem::eToggle, false);
    EXPECT_TRUE(Node_isSelected(smallBrush));
    GlobalSelectionSystem().selectPoint(smallBrushTest, selection::SelectionSystem::eToggle, false);
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
        GlobalSelectionSystem().selectPoint(test, selection::SelectionSystem::eToggle, false);

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
        algorithm::constructCameraView(view, objectAABB, Vector3(0, 0, -1), Vector3(-90, 0, 0));
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

void performPointSelectionOnNodePosition(const scene::INodePtr& node, selection::SelectionSystem::EModifier modifier)
{
    auto nodePosition = node->worldAABB().getOrigin();

    // Construct an orthoview centered at the node's location
    render::View view(false);
    algorithm::constructCenteredOrthoview(view, nodePosition);
    auto test = algorithm::constructOrthoviewSelectionTest(view);

    GlobalSelectionSystem().selectPoint(test, modifier, false);
}

void expectNodeSelectionStatus(const std::vector<scene::INodePtr>& shouldBeSelected, 
    const std::vector<scene::INodePtr>& shouldBeUnselected)
{
    for (const auto& node : shouldBeSelected)
    {
        EXPECT_TRUE(Node_isSelected(node)) << "Node " << node->name() << " should be selected";
    }

    for (const auto& node : shouldBeUnselected)
    {
        EXPECT_FALSE(Node_isSelected(node)) << "Node " << node->name() << " should be unselected";
    }
}

// Ortho: Toggle worldspawn brush selection in primitive mode
TEST_F(OrthoViewSelectionTest, ToggleSelectPointPrimitiveMode)
{
    loadMap("selection_test2.map");

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/2");
    auto unrelatedBrush = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/1");

    expectNodeSelectionStatus({}, { brush, unrelatedBrush });

    performPointSelectionOnNodePosition(brush, selection::SelectionSystem::eToggle);
    expectNodeSelectionStatus({ brush  }, { unrelatedBrush });

    performPointSelectionOnNodePosition(brush, selection::SelectionSystem::eToggle);
    expectNodeSelectionStatus({}, { brush, unrelatedBrush });
}

// Ortho: Toggle selection in primitive mode favours entities over brushes
TEST_F(OrthoViewSelectionTest, ToggleSelectPointPrimitiveModeFavoursEntities)
{
    loadMap("selection_test2.map");

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto funcStatic = algorithm::getEntityByName(GlobalMapModule().getRoot(), "func_static_1");
    auto brush = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/1");

    // Assume that the entity is located below the brush
    Vector3 funcStaticOrigin = funcStatic->worldAABB().getOrigin();
    Vector3 originalBrushPosition = brush->worldAABB().getOrigin();
    EXPECT_LT(funcStaticOrigin.z() + funcStatic->worldAABB().extents.z(), 
        originalBrushPosition.z() + brush->worldAABB().extents.z()) << "Entity should be located below the brush";

    expectNodeSelectionStatus({}, { brush, funcStatic });

    performPointSelectionOnNodePosition(brush, selection::SelectionSystem::eToggle);
    expectNodeSelectionStatus({ funcStatic  }, { brush });

    performPointSelectionOnNodePosition(brush, selection::SelectionSystem::eToggle);
    expectNodeSelectionStatus({}, { funcStatic, brush });
}

// Ortho: Replace selection in primitive mode, check that current selection is replaced
TEST_F(OrthoViewSelectionTest, ReplaceSelectPointPrimitiveMode)
{
    loadMap("selection_test2.map");

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto funcStatic = algorithm::getEntityByName(GlobalMapModule().getRoot(), "func_static_1");
    auto brush1 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/1");
    auto brush2 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/2");

    expectNodeSelectionStatus({}, { brush1, brush2, funcStatic });

    // Select the func_static_1
    Node_setSelected(brush1, true);
    Node_setSelected(funcStatic, true);

    // Run selection in replace mode, this should unselect the previous items
    performPointSelectionOnNodePosition(brush2, selection::SelectionSystem::eReplace);
    expectNodeSelectionStatus({ brush2  }, { brush1, funcStatic });
}

// Ortho: Cycle selection in primitive mode (brush 1 on top of brush 3 on top of a func_static)
TEST_F(OrthoViewSelectionTest, CycleSelectPointPrimitiveMode)
{
    loadMap("selection_test2.map");

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto funcStatic = algorithm::getEntityByName(GlobalMapModule().getRoot(), "func_static_above_torches");
    auto torch1 = algorithm::getEntityByName(GlobalMapModule().getRoot(), "torch1");
    auto torch2 = algorithm::getEntityByName(GlobalMapModule().getRoot(), "torch2");
    auto brush4 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/4");
    auto patch4 = algorithm::findFirstPatchWithMaterial(worldspawn, "textures/numbers/4");

    // Everything should be deselected
    expectNodeSelectionStatus({}, { brush4, patch4, funcStatic, torch1, torch2 });

    // First selection in replace mode should select the func_static, since entities are favoured
    performPointSelectionOnNodePosition(torch1, selection::SelectionSystem::eReplace);
    expectNodeSelectionStatus({ funcStatic }, { brush4, patch4, torch1, torch2 });

    // Second cycle should have the next entity selected
    performPointSelectionOnNodePosition(torch1, selection::SelectionSystem::eCycle);
    expectNodeSelectionStatus({ torch1 }, { brush4, patch4, funcStatic, torch2 });

    // Third cycle should select the brush in between the func_static and brush 1
    performPointSelectionOnNodePosition(torch1, selection::SelectionSystem::eCycle);
    expectNodeSelectionStatus({ torch2 }, { brush4, patch4, funcStatic, torch1 });

    // Fourth cycle should select topmost primitive
    performPointSelectionOnNodePosition(torch1, selection::SelectionSystem::eCycle);
    expectNodeSelectionStatus({ brush4 }, { patch4, funcStatic, torch1, torch2 });

    // Fifth cycle should select the second primitive
    performPointSelectionOnNodePosition(torch1, selection::SelectionSystem::eCycle);
    expectNodeSelectionStatus({ patch4 }, { brush4, funcStatic, torch1, torch2 });

    // Sixth cycle should select the first entity again
    performPointSelectionOnNodePosition(torch1, selection::SelectionSystem::eCycle);
    expectNodeSelectionStatus({ funcStatic }, { brush4, patch4, torch1, torch2 });
}

// Cycle select is not changing the selection if there's only one selectable in the pool
TEST_F(OrthoViewSelectionTest, CycleSelectPointOnlyOneCandidate)
{
    loadMap("selection_test2.map");

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush2 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/2");

    EXPECT_FALSE(Node_isSelected(brush2)) << "Brush 2 should be unselected at first";

    // First selection in replace mode should select brush 2
    performPointSelectionOnNodePosition(brush2, selection::SelectionSystem::eReplace);
    EXPECT_TRUE(Node_isSelected(brush2)) << "brush 2 should be selected now";

    // Second cycle should have the topmost brush selected
    performPointSelectionOnNodePosition(brush2, selection::SelectionSystem::eCycle);
    EXPECT_TRUE(Node_isSelected(brush2)) << "brush 2 should remain selected";
    performPointSelectionOnNodePosition(brush2, selection::SelectionSystem::eCycle);
    EXPECT_TRUE(Node_isSelected(brush2)) << "brush 2 should remain selected";
}

// Ortho: Toggle point selection in entity mode
TEST_F(OrthoViewSelectionTest, ToggleSelectPointEntityMode)
{
    loadMap("selection_test2.map");

    GlobalSelectionSystem().setSelectionMode(selection::SelectionMode::Entity);

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto funcStatic = algorithm::getEntityByName(GlobalMapModule().getRoot(), "func_static_above_torches");
    auto torch1 = algorithm::getEntityByName(GlobalMapModule().getRoot(), "torch1");

    // These two are positioned right above the torches and the func_static, but are worldspawn primitives
    auto brush4 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/4");
    auto patch4 = algorithm::findFirstPatchWithMaterial(worldspawn, "textures/numbers/4");

    expectNodeSelectionStatus({}, { funcStatic, torch1, brush4, patch4 });

    // Since the func_static is located above the torch, it should get selected first
    // The worldspawn primitives should be ignored
    performPointSelectionOnNodePosition(torch1, selection::SelectionSystem::eToggle);
    expectNodeSelectionStatus({ funcStatic  }, { torch1, brush4, patch4 });

    performPointSelectionOnNodePosition(torch1, selection::SelectionSystem::eToggle);
    expectNodeSelectionStatus({}, { funcStatic, torch1, brush4, patch4 });
}

// Ortho: Cycle point selection in entity mode
TEST_F(OrthoViewSelectionTest, CycleSelectPointEntityMode)
{
    loadMap("selection_test2.map");

    GlobalSelectionSystem().setSelectionMode(selection::SelectionMode::Entity);

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto funcStatic = algorithm::getEntityByName(GlobalMapModule().getRoot(), "func_static_above_torches");
    auto torch1 = algorithm::getEntityByName(GlobalMapModule().getRoot(), "torch1");
    auto torch2 = algorithm::getEntityByName(GlobalMapModule().getRoot(), "torch2");

    expectNodeSelectionStatus({}, { funcStatic, torch1, torch2 });

    // First selection in replace mode should select the func_static, since entities are favoured
    performPointSelectionOnNodePosition(torch1, selection::SelectionSystem::eReplace);
    expectNodeSelectionStatus({ funcStatic }, { torch1, torch2 });

    // Second cycle should have the topmost torch selected
    performPointSelectionOnNodePosition(torch1, selection::SelectionSystem::eCycle);
    expectNodeSelectionStatus({ torch1 }, { funcStatic, torch2 });

    // Third cycle should have the second torch selected
    performPointSelectionOnNodePosition(torch1, selection::SelectionSystem::eCycle);
    expectNodeSelectionStatus({ torch2 }, { torch1, funcStatic });

    // Fourth cycle should select func_static again
    performPointSelectionOnNodePosition(torch1, selection::SelectionSystem::eCycle);
    expectNodeSelectionStatus({ funcStatic }, { torch1, torch2 });
}

// Ortho: Replace selection in entity mode, check that current selection is replaced
TEST_F(OrthoViewSelectionTest, ReplaceSelectPointEntityMode)
{
    loadMap("selection_test2.map");

    GlobalSelectionSystem().setSelectionMode(selection::SelectionMode::Entity);

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto funcStatic = algorithm::getEntityByName(GlobalMapModule().getRoot(), "func_static_1");
    auto funcStaticAboveTorches = algorithm::getEntityByName(GlobalMapModule().getRoot(), "func_static_above_torches");

    expectNodeSelectionStatus({}, { funcStaticAboveTorches, funcStatic });

    // Select the func_static_1
    Node_setSelected(funcStatic, true);

    // Run selection in replace mode, this should unselect the previous items
    performPointSelectionOnNodePosition(funcStaticAboveTorches, selection::SelectionSystem::eReplace);
    expectNodeSelectionStatus({ funcStaticAboveTorches }, { funcStatic });
}

TEST_F(OrthoViewSelectionTest, ToggleSelectPointGroupPartMode)
{
    loadMap("selection_test2.map");

    GlobalSelectionSystem().setSelectionMode(selection::SelectionMode::GroupPart);

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto funcStatic = algorithm::getEntityByName(GlobalMapModule().getRoot(), "func_static_1");
    auto funcStatic2 = algorithm::getEntityByName(GlobalMapModule().getRoot(), "func_static_above_torches");
    auto funcStaticBrush = algorithm::findFirstBrushWithMaterial(funcStatic, "textures/numbers/1");
    auto funcStatic2Brush = algorithm::findFirstBrushWithMaterial(funcStatic2, "textures/numbers/1");
    auto torch1 = algorithm::getEntityByName(GlobalMapModule().getRoot(), "torch1");

    expectNodeSelectionStatus({}, { funcStatic, funcStatic2, funcStaticBrush, funcStatic2Brush, torch1 });

    // The torch sits below the func_static_above_torches, only their child brush will be selected
    performPointSelectionOnNodePosition(torch1, selection::SelectionSystem::eToggle);
    expectNodeSelectionStatus({ funcStatic2Brush }, { funcStatic, funcStatic2, funcStaticBrush, torch1 });

    performPointSelectionOnNodePosition(torch1, selection::SelectionSystem::eToggle);
    expectNodeSelectionStatus({}, { funcStatic, funcStatic2, funcStaticBrush, funcStatic2Brush, torch1 });
}

TEST_F(OrthoViewSelectionTest, ReplaceSelectPointGroupPartMode)
{
    loadMap("selection_test2.map");

    GlobalSelectionSystem().setSelectionMode(selection::SelectionMode::GroupPart);

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto funcStatic = algorithm::getEntityByName(GlobalMapModule().getRoot(), "func_static_1");
    auto funcStatic2 = algorithm::getEntityByName(GlobalMapModule().getRoot(), "func_static_above_torches");
    auto funcStaticBrush = algorithm::findFirstBrushWithMaterial(funcStatic, "textures/numbers/1");
    auto funcStatic2Brush = algorithm::findFirstBrushWithMaterial(funcStatic2, "textures/numbers/1");

    expectNodeSelectionStatus({}, { funcStatic, funcStatic2, funcStaticBrush, funcStatic2Brush });

    Node_setSelected(funcStatic2Brush, true);

    // Select the other child primitive of the other func_static, it should replace the selection
    performPointSelectionOnNodePosition(funcStatic, selection::SelectionSystem::eReplace);
    expectNodeSelectionStatus({ funcStaticBrush }, { funcStatic, funcStatic2, funcStatic2Brush });
}

TEST_F(OrthoViewSelectionTest, CycleSelectPointGroupPartMode)
{
    loadMap("selection_test2.map");

    GlobalSelectionSystem().setSelectionMode(selection::SelectionMode::GroupPart);

    auto funcStaticTop = algorithm::getEntityByName(GlobalMapModule().getRoot(), "func_static_top");
    auto funcStaticMiddle = algorithm::getEntityByName(GlobalMapModule().getRoot(), "func_static_middle");
    auto funcStaticBottom = algorithm::getEntityByName(GlobalMapModule().getRoot(), "func_static_bottom");
    auto topBrush = algorithm::findFirstBrushWithMaterial(funcStaticTop, "textures/numbers/1");
    auto middleBrush = algorithm::findFirstBrushWithMaterial(funcStaticMiddle, "textures/numbers/2");
    auto bottomBrush = algorithm::findFirstBrushWithMaterial(funcStaticBottom, "textures/numbers/3");

    // Everything should be unselected at first
    expectNodeSelectionStatus({}, { funcStaticTop, funcStaticMiddle, funcStaticBottom, topBrush, middleBrush, bottomBrush });

    // First selection in replace mode should select the top brush
    performPointSelectionOnNodePosition(topBrush, selection::SelectionSystem::eReplace);
    expectNodeSelectionStatus({ topBrush }, { funcStaticTop, funcStaticMiddle, funcStaticBottom, middleBrush, bottomBrush });

    // Second cycle should have the middle brush selected
    performPointSelectionOnNodePosition(topBrush, selection::SelectionSystem::eCycle);
    expectNodeSelectionStatus({ middleBrush }, { funcStaticTop, funcStaticMiddle, funcStaticBottom, topBrush, bottomBrush });

    // Third cycle should have the bottom brush selected
    performPointSelectionOnNodePosition(topBrush, selection::SelectionSystem::eCycle);
    expectNodeSelectionStatus({ bottomBrush }, { funcStaticTop, funcStaticMiddle, funcStaticBottom, topBrush, middleBrush });

    // Fourth cycle should select the top brush again
    performPointSelectionOnNodePosition(topBrush, selection::SelectionSystem::eCycle);
    expectNodeSelectionStatus({ topBrush }, { funcStaticTop, funcStaticMiddle, funcStaticBottom, middleBrush, bottomBrush });
}

// Ortho: Move two brushes using the drag manipulator
TEST_F(OrthoViewSelectionTest, DragManipulationByDirectHit)
{
    loadMap("selection_test2.map");

    GlobalSelectionSystem().setSelectionMode(selection::SelectionMode::Primitive);
    GlobalSelectionSystem().setActiveManipulator(selection::IManipulator::Drag);

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/1");
    auto brush2 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/2");
    auto manipulator = GlobalSelectionSystem().getActiveManipulator();

    // Select the brushes and attempt to drag-manipulate them
    Node_setSelected(brush, true);
    Node_setSelected(brush2, true);

    auto originalAABB = brush->worldAABB();
    auto originalAABB2 = brush2->worldAABB();

    // Construct a selection test centered at the node's location
    render::View view(false);
    algorithm::constructCenteredOrthoview(view, originalAABB.getOrigin());
    auto test = algorithm::constructOrthoviewSelectionTest(view);

    auto pivot2World = GlobalSelectionSystem().getPivot2World();

    // Hit the brush right at its center, this should perform a drag-selection
    manipulator->testSelect(test, pivot2World);

    EXPECT_TRUE(manipulator->isSelected());
    EXPECT_TRUE(manipulator->getActiveComponent());

    GlobalSelectionSystem().onManipulationStart();
    manipulator->getActiveComponent()->beginTransformation(pivot2World, view, Vector2(0, 0));
    manipulator->getActiveComponent()->transform(pivot2World, view, Vector2(0.5, 0.5), 0);
    GlobalSelectionSystem().onManipulationChanged();

    // The brush should have moved
    auto newAABB = brush->worldAABB();
    auto newAABB2 = brush2->worldAABB();

    EXPECT_FALSE(math::isNear(originalAABB.getOrigin(), newAABB.getOrigin(), 20)) << "Brush should have moved";
    EXPECT_FALSE(math::isNear(originalAABB2.getOrigin(), newAABB2.getOrigin(), 20)) << "Brush 2 should have moved";
    EXPECT_TRUE(math::isNear(originalAABB.getExtents(), newAABB.getExtents(), 0.01)) << "Brush should not have changed form";
    EXPECT_TRUE(math::isNear(originalAABB2.getExtents(), newAABB2.getExtents(), 0.01)) << "Brush 2 should not have changed form";

    EXPECT_TRUE(math::isNear(originalAABB2.getOrigin() - originalAABB.getOrigin(),
        newAABB2.getOrigin() - newAABB.getOrigin(), 0.01)) << "Relative brush position should not have changed";
}

// Ortho: Resize two brushes using the drag manipulator
TEST_F(OrthoViewSelectionTest, DragManipulationByPlane)
{
    loadMap("selection_test2.map");

    GlobalSelectionSystem().setSelectionMode(selection::SelectionMode::Primitive);
    GlobalSelectionSystem().setActiveManipulator(selection::IManipulator::Drag);

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/1");
    auto brush2 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/2");
    auto manipulator = GlobalSelectionSystem().getActiveManipulator();

    // Select both brushes and attempt to drag-manipulate them
    Node_setSelected(brush, true);
    Node_setSelected(brush2, true);

    auto originalAABB = brush->worldAABB();
    auto originalAABB2 = brush2->worldAABB();

    auto selectionBounds = GlobalSelectionSystem().getWorkZone().bounds;

    // Construct a selection test at a point a few pixels off the node's bounding box
    render::View view(false);
    algorithm::constructCenteredOrthoview(view, selectionBounds.getOrigin() + Vector3(selectionBounds.getExtents().x(), 0, 0) + 40);
    auto test = algorithm::constructOrthoviewSelectionTest(view);

    auto pivot2World = GlobalSelectionSystem().getPivot2World();

    // Perform a drag-selection
    manipulator->testSelect(test, pivot2World);

    EXPECT_TRUE(manipulator->isSelected());
    EXPECT_TRUE(manipulator->getActiveComponent());

    GlobalSelectionSystem().onManipulationStart();
    manipulator->getActiveComponent()->beginTransformation(pivot2World, view, Vector2(0, 0));
    manipulator->getActiveComponent()->transform(pivot2World, view, Vector2(0.3, 0), 0);
    GlobalSelectionSystem().onManipulationChanged();

    // The brush should have been resized
    auto newAABB = brush->worldAABB();
    auto newAABB2 = brush2->worldAABB();

    // The extents of the brush should have been changed
    EXPECT_FALSE(math::isNear(originalAABB.getExtents(), newAABB.getExtents(), 20)) << "Brush should have changed form";
    EXPECT_FALSE(math::isNear(originalAABB2.getExtents(), newAABB2.getExtents(), 20)) << "Brush 2 should have changed form";
}

}
