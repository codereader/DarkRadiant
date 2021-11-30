#include "RadiantTest.h"

#include "ieclass.h"
#include "ientity.h"
#include "itransformable.h"
#include "icommandsystem.h"
#include "scenelib.h"
#include "selection/SingleItemSelector.h"
#include "selection/SelectedPlaneSet.h"
#include "render/View.h"
#include "algorithm/View.h"

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

}
