#include "RadiantTest.h"

#include <sigc++/trackable.h>
#include <map>
#include "icommandsystem.h"
#include "itransformable.h"
#include "algorithm/Scene.h"
#include "iselectable.h"
#include "selection/EntitySelection.h"
#include "algorithm/Entity.h"
#include "scenelib.h"

namespace test
{

namespace
{

scene::INodePtr selectEntity(const std::string& name)
{
    auto parent = GlobalMapModule().getRoot();
    auto entity = algorithm::getEntityByName(parent, name);
    Node_setSelected(entity, true);

    return entity;
}

// Helper class representing the list of key values shown in the entity inspector
class KeyValueStore :
    public sigc::trackable
{
private:
    std::unique_ptr<selection::CollectiveSpawnargs> _spawnargs;
    std::unique_ptr<selection::EntitySelection> _selectionTracker;

    std::size_t _additions;
    std::size_t _removals;
    std::size_t _changes;

public:
    static constexpr const char* DifferingValues = "[differing values]";

    KeyValueStore()
    {
        resetEventCounters();

        _spawnargs.reset(new selection::CollectiveSpawnargs);
        _selectionTracker.reset(new selection::EntitySelection(*_spawnargs));

        _spawnargs->signal_KeyAdded().connect(
            sigc::mem_fun(this, &KeyValueStore::onKeyAdded)
        );
        _spawnargs->signal_KeyRemoved().connect(
            sigc::mem_fun(this, &KeyValueStore::onKeyRemoved)
        );
        _spawnargs->signal_KeyValueSetChanged().connect(
            sigc::mem_fun(this, &KeyValueStore::onKeyValueSetChanged)
        );
    }

    ~KeyValueStore()
    {
        _selectionTracker.reset();
        _spawnargs.reset();
    }

    std::map<std::string, std::string> store;

    std::size_t getNumSelectedEntities()
    {
        return _selectionTracker->size();
    }

    void rescanSelection()
    {
        _selectionTracker->update();
    }

    std::size_t getEventCounter() const
    {
        return _additions + _removals + _changes;
    }

    std::size_t getNumAdditions() const
    {
        return _additions;
    }

    std::size_t getNumRemovals() const
    {
        return _removals;
    }

    std::size_t getNumChanges() const
    {
        return _changes;
    }

    void resetEventCounters()
    {
        _additions = 0;
        _removals = 0;
        _changes = 0;
    }

private:
    void onKeyAdded(const std::string& key, const std::string& value)
    {
        ++_additions;
        store[key] = value;
    }

    void onKeyRemoved(const std::string& key)
    {
        ++_removals;
        store.erase(key);
    }

    void onKeyValueSetChanged(const std::string& key, const std::string& uniqueValue)
    {
        ++_changes;
        store[key] = uniqueValue.empty() ? DifferingValues : uniqueValue;
    }
};

inline void expectUnique(const KeyValueStore& keyValueStore, const std::string& key, const std::string& value)
{
    if (keyValueStore.store.count(key) == 0)
    {
        FAIL() << "Key Value Store doesn't contain " << key << " = " << value;
        return;
    }

    EXPECT_EQ(keyValueStore.store.at(key), value) <<
        "Key Value Store should contain " << key << " = " << value << ", but value was " << keyValueStore.store.at(key);
}

inline void expectNonUnique(const KeyValueStore& keyValueStore, const std::string& key)
{
    if (keyValueStore.store.count(key) == 0)
    {
        FAIL() << "Key Value Store doesn't contain " << key;
        return;
    }

    EXPECT_EQ(keyValueStore.store.at(key), KeyValueStore::DifferingValues) <<
        "Key Value Store should contain " << key << " = " << KeyValueStore::DifferingValues <<
        ", but value was " << keyValueStore.store.at(key);
}

inline void expectNotListed(const KeyValueStore& keyValueStore, const std::string& key)
{
    EXPECT_EQ(keyValueStore.store.count(key), 0) <<
        "Key Value Store should not contain " << key;
}

}

using EntityInspectorTest = RadiantTest;

TEST_F(EntityInspectorTest, EmptySelection)
{
    KeyValueStore keyValueStore;
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/entityinspector.map"));

    keyValueStore.rescanSelection();

    EXPECT_TRUE(keyValueStore.store.empty()) << "Shouldn't have any key values without selection";
}

TEST_F(EntityInspectorTest, DeselectSingleSelectedEntity)
{
    KeyValueStore keyValueStore;
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/entityinspector.map"));

    selectEntity("light_torchflame_1");
    keyValueStore.rescanSelection();
    EXPECT_FALSE(keyValueStore.store.empty());

    GlobalSelectionSystem().setSelectedAll(false);
    keyValueStore.rescanSelection();
    EXPECT_TRUE(keyValueStore.store.empty()) << "Shouldn't have any key values without selection";
}

// #5824: Don't trigger change events when all entities are deselected at once
// This prevents the entity inspector from writing the last remaining (and therefore unique)
// value from being written to the value entry box.
TEST_F(EntityInspectorTest, DeselectAllEntitiesAtOnce)
{
    KeyValueStore keyValueStore;
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/entityinspector.map"));

    selectEntity("light_torchflame_1");
    selectEntity("light_torchflame_2");
    selectEntity("light_torchflame_3");
    keyValueStore.rescanSelection();

    // Shared keys, but differing values
    expectNonUnique(keyValueStore, "canBeBlownOut");
    expectNonUnique(keyValueStore, "name");
    expectNonUnique(keyValueStore, "origin");

    // Reset the counters and deselect all entities
    auto numShownKeys = keyValueStore.store.size();
    keyValueStore.resetEventCounters();
    GlobalSelectionSystem().setSelectedAll(false);
    keyValueStore.rescanSelection();

    // The event counters should prove that no change events were received, only removals
    EXPECT_TRUE(keyValueStore.store.empty()) << "Shouldn't have any key values without selection";
    EXPECT_EQ(keyValueStore.getNumAdditions(), 0) << "No additions whatsoever";
    EXPECT_EQ(keyValueStore.getNumChanges(), 0) << "The listener shouldn't have received any changes";

    EXPECT_EQ(keyValueStore.getNumRemovals(), numShownKeys) << "The listener should have received an exact number of removals";
}

inline void assumeLightTorchflame1Spawnargs(const KeyValueStore& keyValueStore)
{
    // All of the entity's spawnargs should be present and showing their value
    expectUnique(keyValueStore, "canBeBlownOut", "0");
    expectUnique(keyValueStore, "classname", "light_torchflame");
    expectUnique(keyValueStore, "name", "light_torchflame_1");
    expectUnique(keyValueStore, "origin", "0 -64 64");
    expectUnique(keyValueStore, "light_center", "0 0 0");
    expectUnique(keyValueStore, "light_radius", "126 102 79");
    expectUnique(keyValueStore, "canBeBlownOut", "0");
    expectUnique(keyValueStore, "unique_to_1", "unique_value");
}

TEST_F(EntityInspectorTest, SingleEntitySelection)
{
    KeyValueStore keyValueStore;
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/entityinspector.map"));

    selectEntity("light_torchflame_1");
    keyValueStore.rescanSelection();

    // All of the entity's spawnargs should be present and showing their value
    assumeLightTorchflame1Spawnargs(keyValueStore);
}

TEST_F(EntityInspectorTest, TwoEntitiesSelected)
{
    KeyValueStore keyValueStore;
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/entityinspector.map"));

    selectEntity("light_torchflame_1");
    keyValueStore.rescanSelection();
    assumeLightTorchflame1Spawnargs(keyValueStore);

    selectEntity("light_torchflame_2");
    keyValueStore.rescanSelection();

    // Shared spawnargs with the same values
    expectUnique(keyValueStore, "canBeBlownOut", "0");
    expectUnique(keyValueStore, "classname", "light_torchflame");
    expectUnique(keyValueStore, "light_center", "0 0 0");
    expectUnique(keyValueStore, "light_radius", "126 102 79");

    // Shared keys, but differing values
    expectNonUnique(keyValueStore, "name");
    expectNonUnique(keyValueStore, "origin");

    // These are exclusively present on one of the two entities
    expectNotListed(keyValueStore, "unique_to_1");
    expectNotListed(keyValueStore, "unique_to_2");
}

TEST_F(EntityInspectorTest, ThreeEntitiesSelected)
{
    KeyValueStore keyValueStore;
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/entityinspector.map"));

    selectEntity("light_torchflame_1");
    keyValueStore.rescanSelection();
    assumeLightTorchflame1Spawnargs(keyValueStore);

    selectEntity("light_torchflame_2");
    selectEntity("light_torchflame_3");
    keyValueStore.rescanSelection();

    // Shared spawnargs with the same values
    expectUnique(keyValueStore, "classname", "light_torchflame");
    expectUnique(keyValueStore, "light_center", "0 0 0");
    expectUnique(keyValueStore, "light_radius", "126 102 79");

    // Shared keys, but differing values
    expectNonUnique(keyValueStore, "canBeBlownOut");
    expectNonUnique(keyValueStore, "name");
    expectNonUnique(keyValueStore, "origin");

    // These are exclusively present on one of the three entities
    expectNotListed(keyValueStore, "unique_to_1");
    expectNotListed(keyValueStore, "unique_to_2");
    expectNotListed(keyValueStore, "unique_to_3");
}

TEST_F(EntityInspectorTest, AssimilateKeyValues)
{
    KeyValueStore keyValueStore;
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/entityinspector.map"));

    auto entity1 = selectEntity("light_torchflame_1");
    auto entity2 = selectEntity("light_torchflame_2");
    auto entity3 = selectEntity("light_torchflame_3");
    keyValueStore.rescanSelection();

    // Shared key, but differing values
    expectNonUnique(keyValueStore, "canBeBlownOut");
    expectNotListed(keyValueStore, "unique_to_1");

    // Set one of the two canBeBlownOut keys to "1", the value on entity 2 is still "0"
    Node_getEntity(entity1)->setKeyValue("canBeBlownOut", "1");
    keyValueStore.rescanSelection();
    expectNonUnique(keyValueStore, "canBeBlownOut");

    // Set the remaining value to "1", now all three should be the same
    Node_getEntity(entity2)->setKeyValue("canBeBlownOut", "1");
    keyValueStore.rescanSelection();
    expectUnique(keyValueStore, "canBeBlownOut", "1");
    expectNotListed(keyValueStore, "unique_to_1");

    // unique_to_1 hasn't been present on entity 2 and 3 yet
    Node_getEntity(entity2)->setKeyValue("unique_to_1", "unique_value");
    keyValueStore.rescanSelection();
    expectNotListed(keyValueStore, "unique_to_1");

    Node_getEntity(entity3)->setKeyValue("unique_to_1", "unique_value");
    keyValueStore.rescanSelection();
    expectUnique(keyValueStore, "unique_to_1", "unique_value"); // should be listed now
}

TEST_F(EntityInspectorTest, AssignDifferingKeyValues)
{
    KeyValueStore keyValueStore;
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/entityinspector.map"));

    auto entity1 = selectEntity("light_torchflame_1");
    auto entity2 = selectEntity("light_torchflame_2");
    auto entity3 = selectEntity("light_torchflame_3");
    keyValueStore.rescanSelection();

    expectUnique(keyValueStore, "light_center", "0 0 0");

    // Changing the value should make it appear as non-unique
    Node_getEntity(entity3)->setKeyValue("light_center", "0 0 1");
    keyValueStore.rescanSelection();
    expectNonUnique(keyValueStore, "light_center");
}

TEST_F(EntityInspectorTest, ChangeUniqueKeyValue)
{
    KeyValueStore keyValueStore;
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/entityinspector.map"));

    auto entity1 = selectEntity("light_torchflame_1");
    keyValueStore.rescanSelection();

    expectUnique(keyValueStore, "light_center", "0 0 0");

    // Change the light_center value
    Node_getEntity(entity1)->setKeyValue("light_center", "0 0 1");
    keyValueStore.rescanSelection();

    expectUnique(keyValueStore, "light_center", "0 0 1");
}

TEST_F(EntityInspectorTest, RemoveUniqueKeyValue)
{
    KeyValueStore keyValueStore;
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/entityinspector.map"));

    auto entity1 = selectEntity("light_torchflame_1");
    keyValueStore.rescanSelection();

    expectUnique(keyValueStore, "light_center", "0 0 0");

    // Remove the light_center key from entity 2, it should disappear
    Node_getEntity(entity1)->setKeyValue("light_center", "");
    keyValueStore.rescanSelection();
    expectNotListed(keyValueStore, "light_center");
}

TEST_F(EntityInspectorTest, AddKeyValueToSingleSelectedEntity)
{
    KeyValueStore keyValueStore;
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/entityinspector.map"));

    auto entity1 = selectEntity("light_torchflame_1");
    keyValueStore.rescanSelection();

    assumeLightTorchflame1Spawnargs(keyValueStore);

    // Add a new key, it should appear
    Node_getEntity(entity1)->setKeyValue("custom_key", "do it");
    keyValueStore.rescanSelection();
    expectUnique(keyValueStore, "custom_key", "do it");
}

TEST_F(EntityInspectorTest, RemoveOneSharedKeyValue)
{
    KeyValueStore keyValueStore;
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/entityinspector.map"));

    auto entity1 = selectEntity("light_torchflame_1");
    auto entity2 = selectEntity("light_torchflame_2");
    auto entity3 = selectEntity("light_torchflame_3");
    keyValueStore.rescanSelection();

    expectUnique(keyValueStore, "light_center", "0 0 0");

    // Remove the light_center key from entity 2, it should disappear
    Node_getEntity(entity2)->setKeyValue("light_center", "");
    keyValueStore.rescanSelection();
    expectNotListed(keyValueStore, "light_center");
}

TEST_F(EntityInspectorTest, ReAddOneSharedKeyValue)
{
    KeyValueStore keyValueStore;
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/entityinspector.map"));

    auto entity1 = selectEntity("light_torchflame_1");
    auto entity2 = selectEntity("light_torchflame_2");
    auto entity3 = selectEntity("light_torchflame_3");
    Node_getEntity(entity2)->setKeyValue("light_center", "");
    keyValueStore.rescanSelection();

    // Since entity 2 doesn't have the ligh_center key, it should not be listed
    expectNotListed(keyValueStore, "light_center");

    auto sharedValue = Node_getEntity(entity1)->getKeyValue("light_center");
    Node_getEntity(entity2)->setKeyValue("light_center", sharedValue);
    keyValueStore.rescanSelection();

    // It should be listed again
    expectUnique(keyValueStore, "light_center", "0 0 0");
}

TEST_F(EntityInspectorTest, DeselectOneEntity)
{
    KeyValueStore keyValueStore;
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/entityinspector.map"));

    auto entity1 = selectEntity("light_torchflame_1");
    auto entity2 = selectEntity("light_torchflame_2");
    auto entity3 = selectEntity("light_torchflame_3");
    keyValueStore.rescanSelection();

    // Shared spawnargs with the same values
    expectUnique(keyValueStore, "classname", "light_torchflame");
    // Shared key, but differing values
    expectNonUnique(keyValueStore, "canBeBlownOut");

    // De-select entity3, it should make the canBeBlownOut key value unique
    Node_setSelected(entity3, false);
    keyValueStore.rescanSelection();

    // This one is unique now
    expectUnique(keyValueStore, "canBeBlownOut", "0");
    // The other ones are still listed
    expectUnique(keyValueStore, "classname", "light_torchflame");
}

// Two speakers and one light are selected, the light is deselected which should make "s_shader" visible
TEST_F(EntityInspectorTest, DeselectOneLight)
{
    KeyValueStore keyValueStore;
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/entityinspector.map"));

    auto light1 = selectEntity("light_torchflame_1");
    auto speaker1 = selectEntity("speaker_1");
    auto speaker2 = selectEntity("speaker_2");
    keyValueStore.rescanSelection();

    // Check a few assumptions on the involved spawnwargs
    auto s_shader = Node_getEntity(speaker1)->getKeyValue("s_shader");
    auto s_maxdistance = Node_getEntity(speaker1)->getKeyValue("s_maxdistance");
    auto s_mindistance = Node_getEntity(speaker1)->getKeyValue("s_mindistance");

    EXPECT_EQ(Node_getEntity(speaker2)->getKeyValue("s_shader"), s_shader) << "s_shader should be the same";
    EXPECT_EQ(Node_getEntity(speaker2)->getKeyValue("s_maxdistance"), s_maxdistance) << "s_maxdistance should be the same";
    EXPECT_EQ(Node_getEntity(speaker2)->getKeyValue("s_mindistance"), s_mindistance) << "s_mindistance should be the same";

    EXPECT_TRUE(Node_getEntity(light1)->getKeyValue("s_shader").empty()) << "Light shouldn't have the s_shader key";
    EXPECT_TRUE(Node_getEntity(light1)->getKeyValue("s_maxdistance").empty()) << "Light shouldn't have the s_maxdistance key";
    EXPECT_TRUE(Node_getEntity(light1)->getKeyValue("s_mindistance").empty()) << "Light shouldn't have the s_mindistance key";

    // These are shown with non-unique values
    expectNonUnique(keyValueStore, "classname");
    expectNonUnique(keyValueStore, "name");
    expectNonUnique(keyValueStore, "origin");

    // These are not shown because the light doesn't have them
    expectNotListed(keyValueStore, "s_shader");
    expectNotListed(keyValueStore, "s_maxdistance");
    expectNotListed(keyValueStore, "s_mindistance");

    // De-select light1, it should make the s_shader key value visible
    Node_setSelected(light1, false);
    keyValueStore.rescanSelection();

    // These should be shown now
    expectUnique(keyValueStore, "s_shader", s_shader);
    expectUnique(keyValueStore, "s_maxdistance", s_maxdistance);
    expectUnique(keyValueStore, "s_mindistance", s_mindistance);
}

TEST_F(EntityInspectorTest, SelectEntitiesPlusWorldspawnPrimitive)
{
    KeyValueStore keyValueStore;
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/entityinspector.map"));

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto light1 = selectEntity("light_torchflame_1");
    auto speaker1 = selectEntity("speaker_1");
    keyValueStore.rescanSelection();

    // Prerequisites of worldspawn
    EXPECT_EQ(Node_getEntity(worldspawn)->getKeyValue("name"), "") << "Worldspawn shouldn't have a name";
    EXPECT_EQ(Node_getEntity(worldspawn)->getKeyValue("origin"), "") << "Worldspawn shouldn't have an origin";

    // These are shown with non-unique values
    expectNonUnique(keyValueStore, "classname");
    expectNonUnique(keyValueStore, "name");
    expectNonUnique(keyValueStore, "origin");

    // These are not shown because the light doesn't have them
    expectNotListed(keyValueStore, "s_shader");
    expectNotListed(keyValueStore, "s_maxdistance");
    expectNotListed(keyValueStore, "s_mindistance");
    // These are not shown because the speaker doesn't have them
    expectNotListed(keyValueStore, "light_center");
    expectNotListed(keyValueStore, "light_radius");
    expectNotListed(keyValueStore, "unique_to_1");
    expectNotListed(keyValueStore, "canBeBlownOut");

    // Now select the worldspawn brush
    // The worldspawn has only one classname spawnarg, so the set should be reduced to 1 key
    auto brush1 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/1");
    Node_setSelected(brush1, true);
    keyValueStore.rescanSelection();

    expectNonUnique(keyValueStore, "classname");

    // Worldspawn doesn't have name nor origin, so these should be gone
    expectNotListed(keyValueStore, "name");
    expectNotListed(keyValueStore, "origin");

    // De-select the brush again, the name and origin should show up again
    Node_setSelected(brush1, false);
    keyValueStore.rescanSelection();

    expectNonUnique(keyValueStore, "classname");
    expectNonUnique(keyValueStore, "name");
    expectNonUnique(keyValueStore, "origin");
}

// As result of a drag-selection
TEST_F(EntityInspectorTest, SelectMultipleEntitiesAndWorldspawnPrimitive)
{
    KeyValueStore keyValueStore;
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/entityinspector.map"));

    // Selection contains 2 lights, 2 speakers and one worldspawn brush
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/1");
    Node_setSelected(brush, true);
    auto light1 = selectEntity("light_torchflame_1");
    auto light3 = selectEntity("light_torchflame_3");
    auto speaker1 = selectEntity("speaker_1");
    auto speaker2 = selectEntity("speaker_2");
    keyValueStore.rescanSelection();

    // These are shown with non-unique values
    expectNonUnique(keyValueStore, "classname");
    EXPECT_EQ(keyValueStore.store.size(), 1) << "Too many keys showing up";
}

TEST_F(EntityInspectorTest, UndoRedoKeyValueChange)
{
    KeyValueStore keyValueStore;
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/entityinspector.map"));

    auto entity1Node = selectEntity("light_torchflame_1");
    auto entity1 = Node_getEntity(entity1Node);
    keyValueStore.rescanSelection();

    // All of the entity's spawnargs should be present and showing their value
    assumeLightTorchflame1Spawnargs(keyValueStore);

    auto keyValuesBeforeChange = algorithm::getAllKeyValuePairs(entity1);
    constexpr const char* ChangedValue = "changed_value";
    {
        UndoableCommand cmd("ChangeKeyValue");
        entity1->setKeyValue("unique_to_1", ChangedValue);
    }

    for (const auto& pair : keyValuesBeforeChange)
    {
        EXPECT_EQ(keyValueStore.store[pair.first], pair.first == "unique_to_1" ? ChangedValue : pair.second)
            << "Keyvalues not matching up after change";
    }

    // Undo, this should revert the change and immediately update the store
    GlobalUndoSystem().undo();

    for (const auto& pair : keyValuesBeforeChange)
    {
        EXPECT_EQ(keyValueStore.store[pair.first], pair.second) << "Keyvalues not matching up after undo";
    }

    // Redo and check again
    GlobalUndoSystem().redo();

    for (const auto& pair : keyValuesBeforeChange)
    {
        EXPECT_EQ(keyValueStore.store[pair.first], pair.first == "unique_to_1" ? ChangedValue : pair.second)
            << "Keyvalues not matching up after redo";
    }
}

TEST_F(EntityInspectorTest, UndoRedoKeyValueAdditionRemoval)
{
    KeyValueStore keyValueStore;
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/entityinspector.map"));

    auto entity1Node = selectEntity("light_torchflame_1");
    auto entity1 = Node_getEntity(entity1Node);
    keyValueStore.rescanSelection();

    // All of the entity's spawnargs should be present and showing their value
    assumeLightTorchflame1Spawnargs(keyValueStore);

    constexpr const char* NewKey = "NewKey";
    constexpr const char* NewValue = "NewValue";

    auto keyValuesBeforeChange = algorithm::getAllKeyValuePairs(entity1);
    {
        UndoableCommand cmd("AddAndRemoveKeyValues");

        // Add a new key
        entity1->setKeyValue(NewKey, NewValue);

        // Remove the unique key
        entity1->setKeyValue("unique_to_1", "");
    }

    // Check that the change got reflected
    expectNotListed(keyValueStore, "unique_to_1");
    expectUnique(keyValueStore, NewKey, NewValue);

    for (const auto& pair : keyValuesBeforeChange)
    {
        if (pair.first != "unique_to_1")
        {
            EXPECT_EQ(keyValueStore.store[pair.first], pair.second) << "Keyvalues not matching up after change";
        }
    }

    // Undo, this should revert the two changes
    GlobalUndoSystem().undo();

    expectNotListed(keyValueStore, NewKey);

    // All other values should be restored again
    for (const auto& pair : keyValuesBeforeChange)
    {
        EXPECT_EQ(keyValueStore.store[pair.first], pair.second) << "Keyvalues not matching up after undo";
    }

    // Redo and check the opposite
    GlobalUndoSystem().redo();

    expectNotListed(keyValueStore, "unique_to_1");
    expectUnique(keyValueStore, NewKey, NewValue);

    for (const auto& pair : keyValuesBeforeChange)
    {
        if (pair.first != "unique_to_1")
        {
            EXPECT_EQ(keyValueStore.store[pair.first], pair.second) << "Keyvalues not matching up after redo";
        }
    }
}

TEST_F(EntityInspectorTest, DeletedEntitiesAreSafelyUntracked)
{
    KeyValueStore keyValueStore;

    // Create a single entity
    auto cls = GlobalEntityClassManager().findClass("atdm:ai_builder_guard");
    auto guardNode = GlobalEntityModule().createEntity(cls);

    // Create a weak reference to check whether the entity is gone
    std::weak_ptr<IEntityNode> weakGuardNode(guardNode);

    scene::addNodeToContainer(guardNode, GlobalMapModule().getRoot());
    Node_setSelected(guardNode, true);

    keyValueStore.rescanSelection();

    EXPECT_FALSE(keyValueStore.store.empty()) << "No key values visible after selecting the entity";
    guardNode.reset();

    // Close the existing map, this should release all references
    GlobalCommandSystem().executeCommand("NewMap");

    EXPECT_TRUE(weakGuardNode.expired()) << "Guard node is still alive after changing maps";
    keyValueStore.rescanSelection();

    EXPECT_TRUE(keyValueStore.store.empty()) << "No key values should be visible after changing maps";
}

// Check behaviour when selecting and deselecting various child primitives of two entities
// every time the entity selection changes, the key value store is supposed to receive callbacks.
TEST_F(EntityInspectorTest, SelectWorldspawnBrushes)
{
    KeyValueStore keyValueStore;
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/entityinspector.map"));

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();

    auto brush1 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/1");
    auto brush2 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/2");
    auto brush3 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/3");
    keyValueStore.rescanSelection();

    // Select one after the other
    Node_setSelected(brush1, true);
    keyValueStore.resetEventCounters();
    keyValueStore.rescanSelection();
    EXPECT_EQ(keyValueStore.getNumSelectedEntities(), 1) << "Expect 1 worldspawn to be selected";
    EXPECT_GT(keyValueStore.getEventCounter(), 0) << "Key value store should have received some events";
    expectUnique(keyValueStore, "classname", "worldspawn");

    Node_setSelected(brush2, true);
    keyValueStore.resetEventCounters();
    keyValueStore.rescanSelection();
    EXPECT_EQ(keyValueStore.getNumSelectedEntities(), 1) << "Expect 1 worldspawn to be selected";
    EXPECT_EQ(keyValueStore.getEventCounter(), 0) << "Key value store should not have received any events";
    expectUnique(keyValueStore, "classname", "worldspawn");

    Node_setSelected(brush3, true);
    keyValueStore.resetEventCounters();
    keyValueStore.rescanSelection();
    EXPECT_EQ(keyValueStore.getNumSelectedEntities(), 1) << "Expect 1 worldspawn to be selected";
    EXPECT_EQ(keyValueStore.getEventCounter(), 0) << "Key value store should not have received any events";
    expectUnique(keyValueStore, "classname", "worldspawn");

    // Deselect again
    Node_setSelected(brush1, false);
    keyValueStore.resetEventCounters();
    keyValueStore.rescanSelection();
    EXPECT_EQ(keyValueStore.getNumSelectedEntities(), 1) << "Expect 1 worldspawn to be selected";
    EXPECT_EQ(keyValueStore.getEventCounter(), 0) << "Key value store should not have received any events";
    expectUnique(keyValueStore, "classname", "worldspawn");

    Node_setSelected(brush3, false);
    keyValueStore.resetEventCounters();
    keyValueStore.rescanSelection();
    EXPECT_EQ(keyValueStore.getNumSelectedEntities(), 1) << "Expect 1 worldspawn to be selected";
    EXPECT_EQ(keyValueStore.getEventCounter(), 0) << "Key value store should not have received any events";
    expectUnique(keyValueStore, "classname", "worldspawn");

    Node_setSelected(brush2, false);
    keyValueStore.resetEventCounters();
    keyValueStore.rescanSelection();
    EXPECT_EQ(keyValueStore.getNumSelectedEntities(), 0) << "Expect nothing to be selected";
    EXPECT_GT(keyValueStore.getEventCounter(), 0) << "Key value store should have received some events";
    expectNotListed(keyValueStore, "classname");

    // Select all at once
    Node_setSelected(brush1, true);
    Node_setSelected(brush2, true);
    Node_setSelected(brush3, true);
    keyValueStore.resetEventCounters();
    keyValueStore.rescanSelection();
    EXPECT_EQ(keyValueStore.getNumSelectedEntities(), 1) << "Expect 1 worldspawn to be selected";
    EXPECT_GT(keyValueStore.getEventCounter(), 0) << "Key value store should have received some events";
    expectUnique(keyValueStore, "classname", "worldspawn");

    // de-select all at once
    Node_setSelected(brush1, false);
    Node_setSelected(brush2, false);
    Node_setSelected(brush3, false);
    keyValueStore.resetEventCounters();
    keyValueStore.rescanSelection();
    EXPECT_EQ(keyValueStore.getNumSelectedEntities(), 0) << "Expect nothing to be selected";
    EXPECT_GT(keyValueStore.getEventCounter(), 0) << "Key value store should have received some events";
    expectNotListed(keyValueStore, "classname");
}

// Check behaviour when selecting and deselecting various child primitives of two entities
// every time the entity selection changes, the key value store is supposed to receive callbacks.
// Even if a primitive selection changes, as long as the corresponding entity selection set
// doesn't change, the key value store should not receive any change events.
TEST_F(EntityInspectorTest, SelectChildPrimitivesOfTwoEntities)
{
    KeyValueStore keyValueStore;
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/entityinspector.map"));

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();

    auto brush1 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/1");
    auto brush2 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/2");
    auto func_static = algorithm::getEntityByName(GlobalMapModule().getRoot(), "func_static_1");
    auto fsBrush4 = algorithm::findFirstBrushWithMaterial(func_static, "textures/numbers/4");
    auto fsBrush5 = algorithm::findFirstBrushWithMaterial(func_static, "textures/numbers/5");

    keyValueStore.rescanSelection();
    EXPECT_EQ(keyValueStore.getNumSelectedEntities(), 0) << "Nothing should be selected at start";

    // Select one after the other
    Node_setSelected(brush1, true);
    keyValueStore.resetEventCounters();
    keyValueStore.rescanSelection();
    EXPECT_EQ(keyValueStore.getNumSelectedEntities(), 1) << "Expect 1 worldspawn to be selected";
    EXPECT_GT(keyValueStore.getEventCounter(), 0) << "Key value store should have received some events";
    expectUnique(keyValueStore, "classname", "worldspawn");

    Node_setSelected(fsBrush4, true);
    keyValueStore.resetEventCounters();
    keyValueStore.rescanSelection();
    EXPECT_EQ(keyValueStore.getNumSelectedEntities(), 2) << "Expect worldspawn and func_static to be selected";
    EXPECT_GT(keyValueStore.getEventCounter(), 0) << "Key value store should have received some events";
    expectNonUnique(keyValueStore, "classname");

    Node_setSelected(brush2, true);
    keyValueStore.resetEventCounters();
    keyValueStore.rescanSelection();
    EXPECT_EQ(keyValueStore.getNumSelectedEntities(), 2) << "Expect worldspawn and func_static to be selected";
    EXPECT_EQ(keyValueStore.getEventCounter(), 0) << "Key value store should not have received any events";
    expectNonUnique(keyValueStore, "classname");

    Node_setSelected(fsBrush5, true);
    keyValueStore.resetEventCounters();
    keyValueStore.rescanSelection();
    EXPECT_EQ(keyValueStore.getNumSelectedEntities(), 2) << "Expect worldspawn and func_static to be selected";
    EXPECT_EQ(keyValueStore.getEventCounter(), 0) << "Key value store should not have received any events";
    expectNonUnique(keyValueStore, "classname");

    // Deselect again
    Node_setSelected(brush2, false);
    keyValueStore.resetEventCounters();
    keyValueStore.rescanSelection();
    EXPECT_EQ(keyValueStore.getNumSelectedEntities(), 2) << "Expect worldspawn and func_static to be selected";
    EXPECT_EQ(keyValueStore.getEventCounter(), 0) << "Key value store should not have received any events";
    expectNonUnique(keyValueStore, "classname");

    Node_setSelected(fsBrush5, false);
    keyValueStore.resetEventCounters();
    keyValueStore.rescanSelection();
    EXPECT_EQ(keyValueStore.getNumSelectedEntities(), 2) << "Expect worldspawn and func_static to be selected";
    EXPECT_EQ(keyValueStore.getEventCounter(), 0) << "Key value store should not have received any events";
    expectNonUnique(keyValueStore, "classname");

    Node_setSelected(brush1, false);
    keyValueStore.resetEventCounters();
    keyValueStore.rescanSelection();
    EXPECT_EQ(keyValueStore.getNumSelectedEntities(), 1) << "Expect nothing to be selected";
    EXPECT_GT(keyValueStore.getEventCounter(), 0) << "Key value store should have received some events";
    expectUnique(keyValueStore, "classname", "func_static");

    Node_setSelected(fsBrush4, false);
    keyValueStore.resetEventCounters();
    keyValueStore.rescanSelection();
    EXPECT_EQ(keyValueStore.getNumSelectedEntities(), 0) << "Expect nothing to be selected";
    EXPECT_GT(keyValueStore.getEventCounter(), 0) << "Key value store should have received some events";
    expectNotListed(keyValueStore, "classname");

    // Select all at once
    Node_setSelected(brush1, true);
    Node_setSelected(brush2, true);
    Node_setSelected(fsBrush4, true);
    Node_setSelected(fsBrush5, true);
    keyValueStore.resetEventCounters();
    keyValueStore.rescanSelection();
    EXPECT_EQ(keyValueStore.getNumSelectedEntities(), 2) << "Expect worldspawn and func_static to be selected";
    EXPECT_GT(keyValueStore.getEventCounter(), 0) << "Key value store should have received some events";
    expectNonUnique(keyValueStore, "classname");

    // de-select all at once
    Node_setSelected(brush1, false);
    Node_setSelected(brush2, false);
    Node_setSelected(fsBrush4, false);
    Node_setSelected(fsBrush5, false);
    keyValueStore.resetEventCounters();
    keyValueStore.rescanSelection();
    EXPECT_EQ(keyValueStore.getNumSelectedEntities(), 0) << "Expect nothing to be selected";
    EXPECT_GT(keyValueStore.getEventCounter(), 0) << "Key value store should have received some events";
    expectNotListed(keyValueStore, "classname");
}

// In a problematic case, a worldspawn patch and a func_static have been moved,
// the modified "origin" key on the func_static was appearing in the Entity Inspector
// after the modification. It should remain hidden, since worldspawn doesn't have an "origin" key.
TEST_F(EntityInspectorTest, MoveFuncStaticAndWorldPrimitives)
{
    KeyValueStore keyValueStore;
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/entityinspector.map"));

    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();

    auto brush1 = algorithm::findFirstBrushWithMaterial(worldspawn, "textures/numbers/1");
    auto func_static = algorithm::getEntityByName(GlobalMapModule().getRoot(), "func_static_1");

    Node_setSelected(brush1, true);
    Node_setSelected(func_static, true);

    keyValueStore.rescanSelection();
    expectNonUnique(keyValueStore, "classname");
    expectNotListed(keyValueStore, "origin");
    EXPECT_EQ(keyValueStore.store.size(), 1) << "Only classname should show up after selection";

    auto previousOrigin = Node_getEntity(func_static)->getKeyValue("origin");

    auto transformable = scene::node_cast<ITransformable>(func_static);
    if (transformable)
    {
        transformable->setType(TRANSFORM_PRIMITIVE);
        transformable->setTranslation(Vector3(10, 0, 0));
        transformable->freezeTransform();
    }

    EXPECT_NE(Node_getEntity(func_static)->getKeyValue("origin"), previousOrigin) << "Origin should have changed";

    keyValueStore.rescanSelection();

    // The spawnarg situation visible in the entity inspector should be the same as before
    // classname is visible, but origin should not be
    expectNonUnique(keyValueStore, "classname");
    expectNotListed(keyValueStore, "origin");
    EXPECT_EQ(keyValueStore.store.size(), 1) << "Only classname should show up after selection";
}

}
