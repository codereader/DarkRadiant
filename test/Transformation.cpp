#include "RadiantTest.h"

#include <algorithm>
#include <vector>
#include "ieclass.h"
#include "ilayer.h"
#include "iselection.h"
#include "iarray.h"
#include "scene/EntityNode.h"
#include "itransformable.h"
#include "icommandsystem.h"
#include "scenelib.h"
#include "selection/SingleItemSelector.h"
#include "selection/SelectedPlaneSet.h"
#include "render/View.h"
#include "algorithm/View.h"
#include "algorithm/Entity.h"
#include "algorithm/Primitives.h"
#include "algorithm/Scene.h"

namespace test
{

using TransformationTest = RadiantTest;

TEST_F(TransformationTest, MoveSelected)
{
    // Create an entity which has editor_mins/editor_maxs defined (GenericEntity)
    auto eclass = GlobalEntityClassManager().findClass("fixed_size_entity");
    auto entityNode = GlobalEntityModule().createEntity(eclass);

    GlobalMapModule().getRoot()->addChildNode(entityNode);

    Node_setSelected(entityNode, true);

    Vector3 originalPosition = entityNode->worldAABB().getOrigin();
    EXPECT_EQ(originalPosition, Vector3(0, 0, 0));

    Vector3 translation(10, 10, 10);
    GlobalCommandSystem().executeCommand("MoveSelection", cmd::Argument(translation));

    EXPECT_EQ(entityNode->worldAABB().getOrigin(), originalPosition + translation);
}

// #5608: Path entities rotate every time when dragged
TEST_F(TransformationTest, TranslationAfterRotatingGenericEntity)
{
    // Repro steps:
    // - New Map
    // - Create Player start somewhere in the ortho view
    // - Dragging the player start won't change the direction of the arrow
    // - Hit "Z-Axis Rotate" to rotate the player start 90 degrees
    // - Dragging the player start will now add another rotation by 90 degrees(every time)

    // Create an entity which has editor_mins/editor_maxs defined (GenericEntity)
    auto eclass = GlobalEntityClassManager().findClass("fixed_size_entity");
    auto entityNode = GlobalEntityModule().createEntity(eclass);

    GlobalMapModule().getRoot()->addChildNode(entityNode);

    Node_setSelected(entityNode, true);

    // Rotate about Z
    GlobalCommandSystem().executeCommand("RotateSelectionZ");

    auto initialAngle = string::convert<int>(entityNode->getEntity().getKeyValue("angle"));
    EXPECT_EQ(initialAngle, -90);

    // Translate as if the thing was dragged around
    GlobalCommandSystem().executeCommand("MoveSelection", cmd::Argument(Vector3(10, 10, 10)));

    auto angleAfterTransformation = string::convert<int>(entityNode->getEntity().getKeyValue("angle"));
    EXPECT_EQ(angleAfterTransformation, initialAngle);
}

scene::INodePtr createAndSelectLight()
{
    // Create an entity which has editor_mins/editor_maxs defined (GenericEntity)
    auto eclass = GlobalEntityClassManager().findClass("light");
    auto entityNode = GlobalEntityModule().createEntity(eclass);

    GlobalMapModule().getRoot()->addChildNode(entityNode);

    Node_setSelected(entityNode, true);

    // Check the prerequisites
    EXPECT_EQ(entityNode->worldAABB(), AABB(Vector3(0, 0, 0), Vector3(320, 320, 320)));

    return entityNode;
}

void selectLightPlaneAt320(const scene::INodePtr& entityNode)
{
    // Select the plane on the "right"
    auto planeSelectable = Node_getPlaneSelectable(entityNode);
    EXPECT_TRUE(planeSelectable);

    // Construct an orthoview to test-select the light
    render::View view(false);
    algorithm::constructCenteredOrthoview(view, Vector3(400, 0, 0));

    SelectionVolume test = algorithm::constructOrthoviewSelectionTest(view);

    selection::SingleItemSelector selector;
    selection::SelectedPlaneSet selectedPlanes;
    planeSelectable->selectPlanes(selector, test, std::bind(&selection::SelectedPlaneSet::insert, &selectedPlanes, std::placeholders::_1));

    EXPECT_FALSE(selectedPlanes.empty()) << "Failed to select the light plane at X=+320";
    EXPECT_TRUE(selectedPlanes.contains(Plane3(1, 0, 0, 320))) << "Failed to select the light plane at X=+320";

    // Select that plane
    selector.getSelectable()->setSelected(true);
}

// #5644: Non uniform light volume scaling not working - the origin is translated ever further off the screen
TEST_F(TransformationTest, NonUniformLightDragResize)
{
    // Disable the symmetric drag-resize mode
    GlobalEntityModule().getSettings().setDragResizeEntitiesSymmetrically(false);

    auto entityNode = createAndSelectLight();
    selectLightPlaneAt320(entityNode);

    auto transformable = scene::node_cast<ITransformable>(entityNode);
    transformable->setType(TRANSFORM_COMPONENT); // we manipulate a component (a plane)
    transformable->setTranslation({ -64, 0, 0 });

    // The light origin should have moved one half of the translation to the left
    EXPECT_EQ(entityNode->worldAABB().getOrigin(), Vector3(-32, 0, 0)); // moved by 32 units
    EXPECT_EQ(entityNode->worldAABB().getExtents(), Vector3(288, 320, 320)); // reduced by 32 units

    // Now set the translation back to 0,0,0, the light should properly reset its transformation
    // to the original state and then apply a 0-length translation, i.e. do nothing
    transformable->setTranslation({ 0, 0, 0 });
    EXPECT_EQ(entityNode->worldAABB().getOrigin(), Vector3(0, 0, 0));
    EXPECT_EQ(entityNode->worldAABB().getExtents(), Vector3(320, 320, 320));
}

TEST_F(TransformationTest, UniformLightDragResize)
{
    // Disable the symmetric drag-resize mode
    GlobalEntityModule().getSettings().setDragResizeEntitiesSymmetrically(true);

    auto entityNode = createAndSelectLight();
    selectLightPlaneAt320(entityNode);

    auto transformable = scene::node_cast<ITransformable>(entityNode);
    transformable->setType(TRANSFORM_COMPONENT); // we manipulate a component (a plane)
    transformable->setTranslation({ -64, 0, 0 });

    // The light origin should not have moved, just the radius should be reduced by 64 on both sides
    EXPECT_EQ(entityNode->worldAABB().getOrigin(), Vector3(0, 0, 0));
    EXPECT_EQ(entityNode->worldAABB().getExtents(), Vector3(320-64, 320, 320)); // reduced by 64 units

    // Now set the translation back to 0,0,0, the light should properly reset its transformation
    // to the original state and then apply a 0-length translation, i.e. do nothing
    transformable->setTranslation({ 0, 0, 0 });
    EXPECT_EQ(entityNode->worldAABB().getOrigin(), Vector3(0, 0, 0));
    EXPECT_EQ(entityNode->worldAABB().getExtents(), Vector3(320, 320, 320));
}

TEST_F(TransformationTest, CloneSelectedPlacesNodeInActiveLayer)
{
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();

    // Create a layer and make it active
    auto testLayerId = layerManager.createLayer("TestLayer");
    layerManager.setActiveLayer(testLayerId);

    // Create an entity on the default layer and select it
    auto entityNode = algorithm::createEntityByClassName("fixed_size_entity");
    GlobalMapModule().getRoot()->addChildNode(entityNode);
    entityNode->moveToLayer(0);
    Node_setSelected(entityNode, true);

    EXPECT_EQ(entityNode->getLayers(), scene::LayerList{ 0 });

    GlobalCommandSystem().executeCommand("CloneSelection");

    EXPECT_EQ(entityNode->getLayers(), scene::LayerList{ 0 });

    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 1);
    GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
    {
        EXPECT_EQ(node->getLayers(), scene::LayerList{ testLayerId })
            << "Cloned node should be placed in the active layer";
    });
}

TEST_F(TransformationTest, CloneSelectedDefaultLayerStaysOnDefault)
{
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();

    EXPECT_EQ(layerManager.getActiveLayer(), 0);

    auto entityNode = algorithm::createEntityByClassName("fixed_size_entity");
    GlobalMapModule().getRoot()->addChildNode(entityNode);
    Node_setSelected(entityNode, true);

    EXPECT_EQ(entityNode->getLayers(), scene::LayerList{ 0 });

    GlobalCommandSystem().executeCommand("CloneSelection");

    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 1);
    GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
    {
        EXPECT_EQ(node->getLayers(), scene::LayerList{ 0 })
            << "Cloned node should be on the default layer when it is active";
    });
}

TEST_F(TransformationTest, CloneSelectedMovesChildrenToActiveLayer)
{
    auto& layerManager = GlobalMapModule().getRoot()->getLayerManager();

    auto testLayerId = layerManager.createLayer("TestLayer");
    layerManager.setActiveLayer(testLayerId);

    auto entityNode = algorithm::createEntityByClassName("func_static");
    GlobalMapModule().getRoot()->addChildNode(entityNode);
    auto brushNode = algorithm::createCubicBrush(entityNode);
    entityNode->moveToLayer(0);
    brushNode->moveToLayer(0);
    Node_setSelected(entityNode, true);

    EXPECT_EQ(entityNode->getLayers(), scene::LayerList{ 0 });
    EXPECT_EQ(brushNode->getLayers(), scene::LayerList{ 0 });

    GlobalCommandSystem().executeCommand("CloneSelection");

    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 1);
    GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
    {
        EXPECT_EQ(node->getLayers(), scene::LayerList{ testLayerId })
            << "Cloned entity should be on the active layer";

        node->foreachNode([&](const scene::INodePtr& child)
        {
            EXPECT_EQ(child->getLayers(), scene::LayerList{ testLayerId })
                << "Cloned child brush should be on the active layer";
            return true;
        });
    });
}

TEST_F(TransformationTest, ArrayCloneLineFixedOffset)
{
    auto eclass = GlobalEntityClassManager().findClass("fixed_size_entity");
    auto entityNode = GlobalEntityModule().createEntity(eclass);

    GlobalMapModule().getRoot()->addChildNode(entityNode);
    Node_setSelected(entityNode, true);

    Vector3 originalPosition = entityNode->worldAABB().getOrigin();
    EXPECT_EQ(originalPosition, Vector3(0, 0, 0));

    // Create 3 copies with a fixed offset of (100, 0, 0), no rotation
    int count = 3;
    int offsetMethod = static_cast<int>(ui::ArrayOffsetMethod::Fixed);
    Vector3 offset(100, 0, 0);
    Vector3 rotation(0, 0, 0);

    GlobalCommandSystem().executeCommand("ArrayCloneSelectionLine",
        { cmd::Argument(count), cmd::Argument(offsetMethod),
          cmd::Argument(offset), cmd::Argument(rotation) });

    // We should have 4 selected items: original + 3 clones
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 4);

    // Collect positions of all selected nodes
    std::vector<Vector3> positions;
    GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
    {
        positions.push_back(node->worldAABB().getOrigin());
    });

    ASSERT_EQ(positions.size(), 4);

    // Sort by X to get deterministic order
    std::sort(positions.begin(), positions.end(),
        [](const Vector3& a, const Vector3& b) { return a.x() < b.x(); });

    EXPECT_TRUE(math::isNear(positions[0], Vector3(0, 0, 0), 0.1))
        << "Original should remain at origin";
    EXPECT_TRUE(math::isNear(positions[1], Vector3(100, 0, 0), 0.1))
        << "First clone should be at offset 1x";
    EXPECT_TRUE(math::isNear(positions[2], Vector3(200, 0, 0), 0.1))
        << "Second clone should be at offset 2x";
    EXPECT_TRUE(math::isNear(positions[3], Vector3(300, 0, 0), 0.1))
        << "Third clone should be at offset 3x";
}

TEST_F(TransformationTest, ArrayCloneLineEndpointOffset)
{
    auto eclass = GlobalEntityClassManager().findClass("fixed_size_entity");
    auto entityNode = GlobalEntityModule().createEntity(eclass);

    GlobalMapModule().getRoot()->addChildNode(entityNode);
    Node_setSelected(entityNode, true);

    // Endpoint mode: offset represents total distance, divided evenly among copies
    int count = 4;
    int offsetMethod = static_cast<int>(ui::ArrayOffsetMethod::Endpoint);
    Vector3 totalOffset(400, 0, 0);
    Vector3 rotation(0, 0, 0);

    GlobalCommandSystem().executeCommand("ArrayCloneSelectionLine",
        { cmd::Argument(count), cmd::Argument(offsetMethod),
          cmd::Argument(totalOffset), cmd::Argument(rotation) });

    // 5 selected: original + 4 clones
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 5);

    std::vector<Vector3> positions;
    GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
    {
        positions.push_back(node->worldAABB().getOrigin());
    });

    std::sort(positions.begin(), positions.end(),
        [](const Vector3& a, const Vector3& b) { return a.x() < b.x(); });

    ASSERT_EQ(positions.size(), 5);

    // Each clone should be offset by totalOffset/count = (100,0,0) * i
    for (int i = 0; i < 5; ++i)
    {
        EXPECT_TRUE(math::isNear(positions[i], Vector3(100.0 * i, 0, 0), 0.1))
            << "Clone " << i << " position mismatch";
    }
}

TEST_F(TransformationTest, ArrayCloneCircle)
{
    auto eclass = GlobalEntityClassManager().findClass("fixed_size_entity");
    auto entityNode = GlobalEntityModule().createEntity(eclass);

    GlobalMapModule().getRoot()->addChildNode(entityNode);
    Node_setSelected(entityNode, true);

    int count = 4;
    double radius = 200.0;
    double startAngle = 0.0;
    double endAngle = 360.0;
    int rotateToCenter = 0;

    GlobalCommandSystem().executeCommand("ArrayCloneSelectionCircle",
        { cmd::Argument(count), cmd::Argument(radius),
          cmd::Argument(startAngle), cmd::Argument(endAngle),
          cmd::Argument(rotateToCenter) });

    // 5 selected: original + 4 clones
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 5);

    // Each clone should be a certain distance from origin
    int clonesAtExpectedRadius = 0;
    GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
    {
        auto pos = node->worldAABB().getOrigin();
        double dist = pos.getLength();

        // The original is at origin, clones within radius
        if (dist > 1.0)
        {
            EXPECT_NEAR(dist, radius, 1.0)
                << "Clone should be at the specified radius";
            ++clonesAtExpectedRadius;
        }
    });

    EXPECT_EQ(clonesAtExpectedRadius, 4) << "All 4 clones should be at the circle radius";
}

}
