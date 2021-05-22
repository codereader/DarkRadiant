#include "RadiantTest.h"

#include "ieclass.h"
#include "ientity.h"
#include "icommandsystem.h"
#include "scenelib.h"

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

}
