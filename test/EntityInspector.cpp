#include "RadiantTest.h"

#include <sigc++/trackable.h>
#include <map>
#include "icommandsystem.h"
#include "algorithm/Scene.h"
#include "iselectable.h"
#include "selection/EntitySelection.h"

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
    std::unique_ptr<selection::EntitySelection> _selectionTracker;

public:
    static constexpr const char* DifferingValues = "[differing values]";

    KeyValueStore()
    {
        _selectionTracker.reset(new selection::EntitySelection);

        _selectionTracker->getSpawnargs().signal_KeyAdded().connect(
            sigc::mem_fun(this, &KeyValueStore::onKeyAdded)
        );
        _selectionTracker->getSpawnargs().signal_KeyRemoved().connect(
            sigc::mem_fun(this, &KeyValueStore::onKeyRemoved)
        );
        _selectionTracker->getSpawnargs().signal_KeyValueSetChanged().connect(
            sigc::mem_fun(this, &KeyValueStore::onKeyValueSetChanged)
        );
    }

    ~KeyValueStore()
    {
        _selectionTracker.reset();
    }

    std::map<std::string, std::string> store;

    selection::CollectiveSpawnargs& getSpawnargs()
    {
        return _selectionTracker->getSpawnargs();
    }

    void rescanSelection()
    {
        _selectionTracker->update();
    }

private:
    void onKeyAdded(const std::string& key, const std::string& value)
    {
        store[key] = value;
    }

    void onKeyRemoved(const std::string& key)
    {
        store.erase(key);
    }

    void onKeyValueSetChanged(const std::string& key, const std::string& uniqueValue)
    {
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

TEST_F(EntityInspectorTest, DeselectAllEntities)
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

    auto sharedValue = Node_getEntity(entity2)->getKeyValue("light_center");
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

}
