#include "RadiantTest.h"

#include <sigc++/connection.h>
#include "iundo.h"
#include "ibrush.h"
#include "ieclass.h"
#include "ientity.h"
#include "iscenegraphfactory.h"
#include "imap.h"
#include "icommandsystem.h"
#include "math/Matrix4.h"
#include "algorithm/Scene.h"
#include "algorithm/Primitives.h"
#include "scenelib.h"
#include "scene/BasicRootNode.h"
#include "testutil/FileSelectionHelper.h"

namespace test
{

using UndoTest = RadiantTest;

TEST_F(UndoTest, UndoSystemFactory)
{
    EXPECT_TRUE(GlobalUndoSystemFactory().createUndoSystem()) << "Undo system factory must not return empty references";
}

TEST_F(UndoTest, EmptyOperation)
{
    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map modified without doing anything";

    // Open an undo operation and finish it without doing anything
    {
        UndoableCommand cmd("test");
    }

    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map should still be modified";
}

TEST_F(UndoTest, BrushCreation)
{
    std::string mapPath = "maps/simple_brushes.map";
    GlobalCommandSystem().executeCommand("OpenMap", mapPath);

    auto material = "textures/numbers/19";
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    EXPECT_FALSE(algorithm::findFirstBrushWithMaterial(worldspawn, material)) << "Test map already has a brush with that material";

    auto brushNode = GlobalBrushCreator().createBrush();
    auto& brush = *Node_getIBrush(brushNode);

    auto translation = Matrix4::getTranslation({ 20, 3, -7 });
    brush.addFace(Plane3(+1, 0, 0, 64).transform(translation));
    brush.addFace(Plane3(-1, 0, 0, 64).transform(translation));
    brush.addFace(Plane3(0, +1, 0, 64).transform(translation));
    brush.addFace(Plane3(0, -1, 0, 64).transform(translation));
    brush.addFace(Plane3(0, 0, +1, 64).transform(translation));
    brush.addFace(Plane3(0, 0, -1, 64).transform(translation));

    brush.setShader("textures/numbers/19");
    brush.evaluateBRep();
    
    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map already modified before the change";

    {
        UndoableCommand cmd("testOperation");
        scene::addNodeToContainer(brushNode, worldspawn);
    }

    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should be modified after this change";
    EXPECT_TRUE(algorithm::findFirstBrushWithMaterial(worldspawn, material)) << "Could not locate the brush";

    // Brush should be gone now
    GlobalUndoSystem().undo();
    EXPECT_FALSE(brushNode->inScene());
    EXPECT_FALSE(algorithm::findFirstBrushWithMaterial(worldspawn, material)) << "Brush should be gone after undo";

    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map should be unchanged again after undo";

    // Redo, brush should be back again
    GlobalUndoSystem().redo();
    EXPECT_TRUE(brushNode->inScene());
    EXPECT_TRUE(algorithm::findFirstBrushWithMaterial(worldspawn, material)) << "Could not locate the brush after redo";

    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should be changed again after redo";
}

namespace
{

constexpr const char* InitialTestKeyValue = "0";

inline scene::INodePtr setupTestEntity()
{
    auto entity = GlobalEntityModule().createEntity(GlobalEntityClassManager().findClass("light"));
    scene::addNodeToContainer(entity, GlobalMapModule().getRoot());
    Node_getEntity(entity)->setKeyValue("test", InitialTestKeyValue);

    return entity;
}

}

TEST_F(UndoTest, AddEntityKeyValue)
{
    auto entity = setupTestEntity();

    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map already modified before the change";

    {
        UndoableCommand cmd("testOperation");
        Node_getEntity(entity)->setKeyValue("newkey", "ljdaslk");
    }

    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should be modified after this change";

    GlobalUndoSystem().undo();
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("newkey"), "");

    GlobalUndoSystem().redo();
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("newkey"), "ljdaslk");
}

TEST_F(UndoTest, ChangeEntityKeyValue)
{
    auto entity = setupTestEntity();

    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map already modified before the change";

    {
        UndoableCommand cmd("testOperation");
        Node_getEntity(entity)->setKeyValue("test", "1");
    }

    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should be modified after this change";

    GlobalUndoSystem().undo();
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test"), InitialTestKeyValue);

    GlobalUndoSystem().redo();
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test"), "1");
}

TEST_F(UndoTest, RemoveEntityKeyValue)
{
    auto entity = setupTestEntity();

    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map already modified before the change";

    {
        UndoableCommand cmd("testOperation");
        Node_getEntity(entity)->setKeyValue("test", "");
    }

    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should be modified after this change";

    GlobalUndoSystem().undo();
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test"), InitialTestKeyValue);

    GlobalUndoSystem().redo();
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test"), "");
}

TEST_F(UndoTest, MultipleChangesInSingleOperation)
{
    auto entity = setupTestEntity();
    auto entity2 = setupTestEntity();

    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map already modified before the change";

    {
        UndoableCommand cmd("testOperation");
        Node_getEntity(entity)->setKeyValue("test", "1");
        Node_getEntity(entity)->setKeyValue("test1", "value1");

        Node_getEntity(entity2)->setKeyValue("test", "2");
        Node_getEntity(entity2)->setKeyValue("test2", "value2");
        Node_getEntity(entity2)->setKeyValue("test3", "value3");
    }

    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should be modified after this change";

    GlobalUndoSystem().undo();
    Node_getEntity(entity)->setKeyValue("test", InitialTestKeyValue);
    Node_getEntity(entity)->setKeyValue("test1", "");

    Node_getEntity(entity2)->setKeyValue("test", InitialTestKeyValue);
    Node_getEntity(entity2)->setKeyValue("test2", "");
    Node_getEntity(entity2)->setKeyValue("test3", "");

    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map should be marked as unchanged again";

    GlobalUndoSystem().redo();
    Node_getEntity(entity)->setKeyValue("test", "1");
    Node_getEntity(entity)->setKeyValue("test1", "value1");

    Node_getEntity(entity2)->setKeyValue("test", "2");
    Node_getEntity(entity2)->setKeyValue("test2", "value2");
    Node_getEntity(entity2)->setKeyValue("test3", "value3");

    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should be modified after redo";
}

TEST_F(UndoTest, NodeOutsideScene)
{
    auto entity = setupTestEntity();
    
    scene::removeNodeFromParent(entity);

    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map already modified before the change";
    EXPECT_FALSE(entity->inScene()) << "Node has been removed from its parent, should be outside the scene";

    {
        UndoableCommand cmd("testOperation");
        Node_getEntity(entity)->setKeyValue("test", "1");
    }

    // The map should not care about this change
    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map should not be modified after this change";

    GlobalUndoSystem().undo();
    // Node should still be unaffected by this
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test"), "1");

    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map should not be modified after this change";

    GlobalUndoSystem().redo();
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test"), "1");

    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map should not be modified after this change";
}

TEST_F(UndoTest, SequentialOperations)
{
    auto entity = setupTestEntity();
    auto entity2 = setupTestEntity();

    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map already modified before the change";

    {
        UndoableCommand cmd("testOperation");
        Node_getEntity(entity)->setKeyValue("test", "1");
    }
    {
        UndoableCommand cmd("testOperation");
        Node_getEntity(entity)->setKeyValue("test1", "value1");
    }
    {
        UndoableCommand cmd("testOperation");
        Node_getEntity(entity2)->setKeyValue("test", "2");
    }
    {
        UndoableCommand cmd("testOperation");
        Node_getEntity(entity2)->setKeyValue("test2", "value2");
    }
    {
        UndoableCommand cmd("testOperation");
        Node_getEntity(entity2)->setKeyValue("test3", "value3");
    }

    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should be modified after this change";

    // Roll back one change after the other

    GlobalUndoSystem().undo();
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test3"), ""); // undone
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test2"), "value2"); // unchanged
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test"), "2"); // unchanged
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test1"), "value1"); // unchanged
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test"), "1"); // unchanged

    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should still be modified after 1 undo";

    GlobalUndoSystem().undo();
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test3"), ""); // undone
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test2"), ""); // undone
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test"), "2"); // unchanged
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test1"), "value1"); // unchanged
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test"), "1"); // unchanged

    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should still be modified after 2 undos";

    GlobalUndoSystem().undo();
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test3"), ""); // undone
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test2"), ""); // undone
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test"), InitialTestKeyValue); // undone
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test1"), "value1"); // unchanged
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test"), "1"); // unchanged

    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should still be modified after 3 undos";

    GlobalUndoSystem().undo();
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test3"), ""); // undone
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test2"), ""); // undone
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test"), InitialTestKeyValue); // undone
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test1"), ""); // undone
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test"), "1"); // unchanged

    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should still be modified after 4 undos";

    GlobalUndoSystem().undo();
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test3"), ""); // undone
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test2"), ""); // undone
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test"), InitialTestKeyValue); // undone
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test1"), ""); // undone
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test"), InitialTestKeyValue); // undone

    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map should be unmodified after 5 undos";

    // Then redo all of these one after one

    GlobalUndoSystem().redo();
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test3"), ""); // undone
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test2"), ""); // undone
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test"), InitialTestKeyValue); // undone
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test1"), ""); // undone
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test"), "1"); // redone

    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should be modified after 1 redo";

    GlobalUndoSystem().redo();
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test3"), ""); // undone
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test2"), ""); // undone
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test"), InitialTestKeyValue); // undone
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test1"), "value1"); // redone
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test"), "1"); // redone

    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should be modified after 2 redos";

    GlobalUndoSystem().redo();
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test3"), ""); // undone
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test2"), ""); // undone
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test"), "2"); // redone
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test1"), "value1"); // redone
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test"), "1"); // redone

    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should be modified after 3 redos";

    GlobalUndoSystem().redo();
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test3"), ""); // undone
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test2"), "value2"); // redone
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test"), "2"); // redone
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test1"), "value1"); // redone
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test"), "1"); // redone

    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should be modified after 4 redos";

    GlobalUndoSystem().redo();
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test3"), "value3"); // redone
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test2"), "value2"); // redone
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test"), "2"); // redone
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test1"), "value1"); // redone
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test"), "1"); // redone

    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should be modified after 5 redos";
}

TEST_F(UndoTest, NestedOperations)
{
    auto entity = setupTestEntity();
    auto entity2 = setupTestEntity();

    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map already modified before the change";

    // A nested set of operations, should be undoable as one single operation
    {
        UndoableCommand cmd("testOperation");
        Node_getEntity(entity)->setKeyValue("test", "1");

        {
            UndoableCommand cmd("testOperation");
            Node_getEntity(entity)->setKeyValue("test1", "value1");
        }
        {
            UndoableCommand cmd("testOperation");
            Node_getEntity(entity2)->setKeyValue("test", "2");

            {
                UndoableCommand cmd("testOperation");
                Node_getEntity(entity2)->setKeyValue("test2", "value2");
            }
        }

        Node_getEntity(entity)->setKeyValue("last", "one");
    }

    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should be modified after this change";

    GlobalUndoSystem().undo();
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("last"), ""); // undone
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test2"), ""); // undone
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test"), InitialTestKeyValue); // undone
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test1"), ""); // undone
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test"), InitialTestKeyValue); // undone

    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map should be unmodified after undo";

    GlobalUndoSystem().redo();
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("last"), "one"); // redone
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test2"), "value2"); // redone
    EXPECT_EQ(Node_getEntity(entity2)->getKeyValue("test"), "2"); // redone
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test1"), "value1"); // redone
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test"), "1"); // redone

    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should be modified after redo";
}

// Changing a key value multiple times within a single operation is treated correctly
TEST_F(UndoTest, MultipleKeyValueChangeInSingleOperation)
{
    auto entity = setupTestEntity();

    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map already modified before the change";

    {
        UndoableCommand cmd("testOperation");
        Node_getEntity(entity)->setKeyValue("test", "1");
        Node_getEntity(entity)->setKeyValue("test", "2");
        Node_getEntity(entity)->setKeyValue("test", "3");
        Node_getEntity(entity)->setKeyValue("test", "4");
    }

    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should be modified after this change";

    GlobalUndoSystem().undo();
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test"), InitialTestKeyValue);

    GlobalUndoSystem().redo();
    EXPECT_EQ(Node_getEntity(entity)->getKeyValue("test"), "4");
}

TEST_F(UndoTest, SceneNodeRemoval)
{
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/entityinspector.map"));

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto speaker_1 = algorithm::getEntityByName(GlobalMapModule().getRoot(), "speaker_1");
    auto func_static = algorithm::getEntityByName(GlobalMapModule().getRoot(), "func_static_1");

    auto childBrush = algorithm::findFirstBrushWithMaterial(func_static, "textures/numbers/4");
    auto worldspawnBrush = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/1");

    // Check the prerequisities
    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map already modified before the change";

    EXPECT_TRUE(speaker_1);
    EXPECT_TRUE(func_static);
    EXPECT_TRUE(childBrush);
    EXPECT_TRUE(worldspawnBrush);

    EXPECT_TRUE(speaker_1->getParent());
    EXPECT_TRUE(func_static->getParent());
    EXPECT_TRUE(childBrush->getParent());
    EXPECT_TRUE(worldspawnBrush->getParent());

    {
        UndoableCommand cmd("testOperation");
        
        // Remove one of the entities
        scene::removeNodeFromParent(speaker_1);

        // Remove a worldspawn brush
        scene::removeNodeFromParent(worldspawnBrush);

        // Remove a child brush from the func_static
        scene::removeNodeFromParent(childBrush);

        // And top that by removing the func_static itself
        scene::removeNodeFromParent(func_static);
    }

    // All nodes should now be without parent
    EXPECT_FALSE(speaker_1->getParent());
    EXPECT_FALSE(func_static->getParent());
    EXPECT_FALSE(childBrush->getParent());
    EXPECT_FALSE(worldspawnBrush->getParent());

    // Lookup the nodes again, they should be gone
    EXPECT_FALSE(algorithm::getEntityByName(GlobalMapModule().getRoot(), "speaker_1"));
    EXPECT_FALSE(algorithm::getEntityByName(GlobalMapModule().getRoot(), "func_static_1"));
    EXPECT_FALSE(algorithm::findFirstBrushWithMaterial(func_static, "textures/numbers/4"));
    EXPECT_FALSE(algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/1"));

    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should be modified after this change";

    GlobalUndoSystem().undo();

    // All nodes should have proper parents now
    EXPECT_TRUE(speaker_1->getParent());
    EXPECT_TRUE(func_static->getParent());
    EXPECT_TRUE(childBrush->getParent());
    EXPECT_TRUE(worldspawnBrush->getParent());

    EXPECT_TRUE(speaker_1->inScene());
    EXPECT_TRUE(func_static->inScene());
    EXPECT_TRUE(childBrush->inScene());
    EXPECT_TRUE(worldspawnBrush->inScene());

    // Lookup the nodes, they should be present again
    EXPECT_TRUE(algorithm::getEntityByName(GlobalMapModule().getRoot(), "speaker_1"));
    EXPECT_TRUE(algorithm::getEntityByName(GlobalMapModule().getRoot(), "func_static_1"));
    EXPECT_TRUE(algorithm::findFirstBrushWithMaterial(func_static, "textures/numbers/4"));
    EXPECT_TRUE(algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/1"));

    GlobalUndoSystem().redo();

    // Well, this is weird, parents are not cleared when importing a TraversableNodeSet from
    // an undo memento, this seems to have been introduced in the fix to #2118
    EXPECT_TRUE(speaker_1->getParent());
    EXPECT_TRUE(func_static->getParent());
    EXPECT_TRUE(childBrush->getParent());
    EXPECT_TRUE(worldspawnBrush->getParent());

    // All nodes should be outside the scene
    EXPECT_FALSE(speaker_1->inScene());
    EXPECT_FALSE(func_static->inScene());
    EXPECT_FALSE(childBrush->inScene());
    EXPECT_FALSE(worldspawnBrush->inScene());

    // At least, they should be gone when trying to look them up through the scene
    EXPECT_FALSE(algorithm::getEntityByName(GlobalMapModule().getRoot(), "speaker_1"));
    EXPECT_FALSE(algorithm::getEntityByName(GlobalMapModule().getRoot(), "func_static_1"));
    // Same oddity here, the child brush didn't get its parent reference cleared during redo
    EXPECT_TRUE(algorithm::findFirstBrushWithMaterial(func_static, "textures/numbers/4"));
    EXPECT_FALSE(algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/1"));
}

// Test corresponding to #2118: Crash when undoing "create entity from brush "atdm:mover_door_sliding"
TEST_F(UndoTest, CreateBrushBasedEntity)
{
    // start DR, create a brush
    // right-click, select "make entity" and select "atdm:mover_door_sliding" => "Ok"
    // Press CTRL-Z to undo - DR crashes
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();

    // Create and select a brush
    auto brush = algorithm::createCubicBrush(worldspawn, { 5, 5, 15 }, "textures/numbers/1");
    Node_setSelected(brush, true);

    // Initially, this brush has the worldspawn as parent
    EXPECT_TRUE(Node_getEntity(brush->getParent())->isWorldspawn());

    EXPECT_TRUE(GlobalEntityClassManager().findClass("atdm:mover_door_sliding")) << "Need atdm:mover_door_sliding for this test";

    IEntityNodePtr entity;
    {
        UndoableCommand command("createEntity");
        entity = GlobalEntityModule().createEntityFromSelection("atdm:mover_door_sliding", Vector3(7.5, 12, 0));
    }

    EXPECT_TRUE(entity);
    EXPECT_TRUE(brush->getParent());
    EXPECT_FALSE(algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/1")) << "Worldspawn should have lost this brush";

    EXPECT_FALSE(Node_getEntity(brush->getParent())->isWorldspawn());
    EXPECT_EQ(Node_getEntity(brush->getParent())->getEntityClass()->getDeclName(), "atdm:mover_door_sliding");

    auto entityName = Node_getEntity(entity)->getKeyValue("name");
    EXPECT_TRUE(algorithm::getEntityByName(GlobalMapModule().getRoot(), entityName)) << "Could not look up the entity by name";

    GlobalUndoSystem().undo();

    // The brush should be back at worldspawn
    EXPECT_TRUE(Node_getEntity(brush->getParent())->isWorldspawn());
    EXPECT_TRUE(algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/1")) << "Worldspawn should have this brush again";
    
    // The entity should be gone
    EXPECT_FALSE(algorithm::getEntityByName(GlobalMapModule().getRoot(), entityName)) << "Could look up the entity by name";
    EXPECT_FALSE(algorithm::findFirstBrushWithMaterial(entity, "textures/numbers/1")) << "The door should have lost this brush";

    GlobalUndoSystem().redo();

    // All should be back in place after redo
    EXPECT_FALSE(algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/1")) << "Worldspawn should have lost this brush";
    EXPECT_TRUE(algorithm::findFirstBrushWithMaterial(entity, "textures/numbers/1")) << "The door should have this brush now";
    EXPECT_FALSE(Node_getEntity(brush->getParent())->isWorldspawn());
    EXPECT_EQ(Node_getEntity(brush->getParent())->getEntityClass()->getDeclName(), "atdm:mover_door_sliding");
    EXPECT_TRUE(algorithm::getEntityByName(GlobalMapModule().getRoot(), entityName)) << "Could not look up the entity by name";
}

TEST_F(UndoTest, MapChangeTracking)
{
    auto entity = setupTestEntity();
    auto entity2 = setupTestEntity();

    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map already modified before the change";

    // Issue 5 commands to the undo stack
    {
        UndoableCommand cmd("testOperation");
        Node_getEntity(entity)->setKeyValue("test", "1");
    }
    {
        UndoableCommand cmd("testOperation");
        Node_getEntity(entity)->setKeyValue("test1", "value1");
    }
    {
        UndoableCommand cmd("testOperation");
        Node_getEntity(entity2)->setKeyValue("test", "2");
    }
    {
        UndoableCommand cmd("testOperation");
        Node_getEntity(entity2)->setKeyValue("test2", "value2");
    }
    {
        UndoableCommand cmd("testOperation");
        Node_getEntity(entity2)->setKeyValue("test3", "value3");
    }

    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should be modified after this change";

    // Select the format based on the extension
    fs::path tempPath = _context.getTemporaryDataPath();
    tempPath /= "just_a_map.map";
    auto format = GlobalMapFormatManager().getMapFormatForFilename(tempPath.string());

    // Respond to the event asking for the target path
    FileSelectionHelper responder(tempPath.string(), format);

    GlobalCommandSystem().executeCommand("SaveMapAs");

    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map should be unmodified after saving";

    // Two undo steps will mark the map as modified
    GlobalUndoSystem().undo();
    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should be modified after 1 undo";
    GlobalUndoSystem().undo();
    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should be modified after 2 undos";

    // Save at this point, this clears the modified status
    GlobalCommandSystem().executeCommand("SaveMap");
    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map should be unmodified after saving";

    // One step back, one step forward
    GlobalUndoSystem().undo();
    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should be modified after 1 undo";
    GlobalUndoSystem().redo();
    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map should be unmodified after 1 redo";

    // Two steps forward, two steps back, map is unmodified again
    GlobalUndoSystem().redo();
    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should be modified after 1 redo";
    GlobalUndoSystem().redo();
    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should be modified after 2 redos";
    GlobalUndoSystem().undo();
    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should be modified after 1 undo";
    GlobalUndoSystem().undo();
    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map should be unmodified after 2 undos";

    // Two more redos, then save
    GlobalUndoSystem().redo();
    GlobalUndoSystem().redo();
    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should be modified after 2 redos";

    GlobalCommandSystem().executeCommand("SaveMap");
    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map should be unmodified after saving";
}

// Changing a second scene should not mark the main scene as modified
TEST_F(UndoTest, TwoSceneGraphs)
{
    // Load a map, it is unmodified
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/entityinspector.map"));
    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map should be unmodified after loading";

    auto scene = GlobalSceneGraphFactory().createSceneGraph();
    auto rootNode = std::make_shared<scene::BasicRootNode>();
    scene->setRoot(rootNode);

    // Crete the host entity
    auto materialName = "textures/numbers/12";
    auto entity = GlobalEntityModule().createEntity(GlobalEntityClassManager().findClass("func_static"));
    scene::addNodeToContainer(entity, rootNode);

    {
        // Create an brush in an Undoable operation
        UndoableCommand cmd("createBrush");
        algorithm::createCubicBrush(entity, Vector3(0,0,0), materialName);
    }

    EXPECT_TRUE(algorithm::findFirstBrushWithMaterial(entity, materialName)) << "Failed to find the brush";

    // This should not have affected the main scene
    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map should be unmodified after loading";

    // Undoing the main scene should not affect the secondary scene
    GlobalUndoSystem().undo();

    // Still no change in the main map
    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map should be unmodified (no undo should be possible)";

    // The brush should still be present in the second scene
    EXPECT_TRUE(algorithm::findFirstBrushWithMaterial(entity, materialName)) << "Failed to find the brush";
}

// Map is firing the undo/redo signals after an operation completes
TEST_F(UndoTest, MapUndoRedoSignals)
{
    auto postUndoFired = false;
    auto postRedoFired = false;

    GlobalMapModule().signal_postUndo().connect([&] { postUndoFired = true; });
    GlobalMapModule().signal_postRedo().connect([&] { postRedoFired = true; });

    // Record one undoable operation
    {
        UndoableCommand cmd("testOperation");
        auto entity = setupTestEntity();
    }

    GlobalUndoSystem().undo();
    EXPECT_TRUE(postUndoFired) << "Map didn't fire the post-undo signal";

    // Reset and undo one more
    postUndoFired = false;
    GlobalUndoSystem().undo();
    EXPECT_FALSE(postUndoFired) << "Map fired the undo signal even though there's nothing to undo";

    GlobalUndoSystem().redo();
    EXPECT_TRUE(postRedoFired) << "Map didn't fire the post-redo signal";

    postRedoFired = false;
    GlobalUndoSystem().redo();
    EXPECT_FALSE(postUndoFired) << "Map fired the redo signal even though there's nothing to redo";
}

TEST_F(UndoTest, MapUndoRedoSignalsWhenChangingMaps)
{
    auto postUndoFired = false;
    auto postRedoFired = false;

    // Subscribe to the signals before changing the map
    GlobalMapModule().signal_postUndo().connect([&] { postUndoFired = true; });
    GlobalMapModule().signal_postRedo().connect([&] { postRedoFired = true; });

    // Record one undoable operation in the existing map
    {
        UndoableCommand cmd("testOperation");
        setupTestEntity();
    }

    auto previousRoot = GlobalMapModule().getRoot();

    // Discard the map and open a new one
    GlobalCommandSystem().execute("NewMap");

    EXPECT_NE(previousRoot, GlobalMapModule().getRoot()) << "Map root didn't change";

    {
        UndoableCommand cmd("testOperation");
        setupTestEntity();
    }

    GlobalUndoSystem().undo();
    EXPECT_TRUE(postUndoFired) << "Map didn't fire the post-undo signal after changing maps";

    GlobalUndoSystem().redo();
    EXPECT_TRUE(postRedoFired) << "Map didn't fire the post-redo signal after changing maps";
}

class TestUndoTracker
{
public:
    TestUndoTracker()
    {
        reset();
    }

    bool recordedFired;
    bool undoneFired;
    bool redoneFired;
    bool clearedFired;
    std::string receivedOperationName;

    void reset()
    {
        recordedFired = false;
        undoneFired = false;
        redoneFired = false;
        clearedFired = false;
        receivedOperationName.clear();
    }

    void onUndoEvent(IUndoSystem::EventType type, const std::string& operationName)
    {
        recordedFired |= type == IUndoSystem::EventType::OperationRecorded;
        undoneFired |= type == IUndoSystem::EventType::OperationUndone;
        redoneFired |= type == IUndoSystem::EventType::OperationRedone;
        clearedFired |= type == IUndoSystem::EventType::AllOperationsCleared;

        receivedOperationName = operationName;
    }
};

TEST_F(UndoTest, OperationTracking)
{
    TestUndoTracker tracker;

    sigc::connection handler = GlobalMapModule().getRoot()->getUndoSystem()
        .signal_undoEvent().connect(sigc::mem_fun(tracker, &TestUndoTracker::onUndoEvent));

    // Record one undoable operation in the existing map
    {
        UndoableCommand cmd("testOperation");
        setupTestEntity();
    }

    EXPECT_TRUE(tracker.recordedFired) << "Expected signal not fired after operation complete";
    EXPECT_FALSE(tracker.undoneFired) << "Wrong signal fired";
    EXPECT_FALSE(tracker.redoneFired) << "Wrong signal fired";
    EXPECT_FALSE(tracker.clearedFired) << "Wrong signal fired";
    EXPECT_EQ(tracker.receivedOperationName, "testOperation") << "Message not correct";

    tracker.reset();
    GlobalUndoSystem().undo();

    EXPECT_TRUE(tracker.undoneFired) << "Expected signal not fired after operation undone";
    EXPECT_FALSE(tracker.recordedFired) << "Wrong signal fired";
    EXPECT_FALSE(tracker.redoneFired) << "Wrong signal fired";
    EXPECT_FALSE(tracker.clearedFired) << "Wrong signal fired";
    EXPECT_EQ(tracker.receivedOperationName, "testOperation") << "Message not correct";

    // Undo again
    tracker.reset();
    GlobalUndoSystem().undo();

    EXPECT_FALSE(tracker.undoneFired) << "Nothing should fire, nothing to undo";
    EXPECT_FALSE(tracker.recordedFired) << "Nothing should fire, nothing to undo";
    EXPECT_FALSE(tracker.redoneFired) << "Nothing should fire, nothing to undo";
    EXPECT_FALSE(tracker.clearedFired) << "Nothing should fire, nothing to undo";
    EXPECT_EQ(tracker.receivedOperationName, "") << "Nothing should fire, nothing to undo";

    tracker.reset();
    GlobalUndoSystem().redo();

    EXPECT_TRUE(tracker.redoneFired) << "Expected signal not fired after operation redone";
    EXPECT_FALSE(tracker.recordedFired) << "Wrong signal fired";
    EXPECT_FALSE(tracker.undoneFired) << "Wrong signal fired";
    EXPECT_FALSE(tracker.clearedFired) << "Wrong signal fired";
    EXPECT_EQ(tracker.receivedOperationName, "testOperation") << "Message not correct";

    tracker.reset();
    GlobalUndoSystem().redo();

    EXPECT_FALSE(tracker.undoneFired) << "Nothing should fire, nothing to redo";
    EXPECT_FALSE(tracker.recordedFired) << "Nothing should fire, nothing to redo";
    EXPECT_FALSE(tracker.redoneFired) << "Nothing should fire, nothing to redo";
    EXPECT_FALSE(tracker.clearedFired) << "Nothing should fire, nothing to redo";
    EXPECT_EQ(tracker.receivedOperationName, "") << "Nothing should fire, nothing to redo";

    GlobalMapModule().getRoot()->getUndoSystem().clear();

    EXPECT_TRUE(tracker.clearedFired) << "Expected signal not fired after operation redone";
    EXPECT_FALSE(tracker.recordedFired) << "Wrong signal fired";
    EXPECT_FALSE(tracker.undoneFired) << "Wrong signal fired";
    EXPECT_FALSE(tracker.redoneFired) << "Wrong signal fired";
    EXPECT_EQ(tracker.receivedOperationName, "") << "Message not correct";

    handler.disconnect();
    tracker.reset();

    // One more change after detaching
    {
        UndoableCommand cmd("testOperation");
        setupTestEntity();
    }

    EXPECT_FALSE(tracker.undoneFired) << "Nothing should fire, already detached";
    EXPECT_FALSE(tracker.recordedFired) << "Nothing should fire, already detached";
    EXPECT_FALSE(tracker.redoneFired) << "Nothing should fire, already detached";
    EXPECT_FALSE(tracker.clearedFired) << "Nothing should fire, already detached";
    EXPECT_EQ(tracker.receivedOperationName, "") << "Nothing should fire, already detached";
}

}
