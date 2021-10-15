#include "RadiantTest.h"

#include <sigc++/trackable.h>
#include "icommandsystem.h"
#include "algorithm/Scene.h"
#include "iselectable.h"
#include "selection/EntitySelection.h"

namespace test
{

namespace
{

void selectEntity(const std::string& name)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto entity = algorithm::getEntityByName(worldspawn, name);
    Node_setSelected(entity, true);
}

// Helper class representing the list of key values shown in the entity inspector
class KeyValueStore :
    public sigc::trackable
{
private:
    selection::EntitySelection _selectionTracker;

public:
    KeyValueStore()
    {
        _selectionTracker.getSpawnargs().signal_KeyAdded().connect(
            sigc::mem_fun(this, &KeyValueStore::onKeyAdded)
        );
    }

    std::map<std::string, std::string> store;

    const selection::CollectiveSpawnargs& getSpawnargs()
    {
        return _selectionTracker.getSpawnargs();
    }

    void rescanSelection()
    {
        _selectionTracker.update();
    }

private:
    void onKeyAdded()
    {

    }
};

}

using EntityInspectorTest = RadiantTest;

TEST_F(EntityInspectorTest, SingleEntitySelection)
{
    KeyValueStore keyValueStore;
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/entityinspector.map"));

    selectEntity("light_torchflame_1");
    keyValueStore.rescanSelection();

    // All of the entity's spawnargs should be present and showing their value
    EXPECT_EQ(keyValueStore.store["canBeBlownOut"], "0");
    EXPECT_EQ(keyValueStore.store["classname"], "light_torchflame");
    EXPECT_EQ(keyValueStore.store["name"], "light_torchflame_1");
    EXPECT_EQ(keyValueStore.store["origin"], "0 -64 64");
    EXPECT_EQ(keyValueStore.store["light_center"], "0 0 0");
    EXPECT_EQ(keyValueStore.store["light_radius"], "126 102 79");
    EXPECT_EQ(keyValueStore.store["canBeBlownOut"], "0");
    EXPECT_EQ(keyValueStore.store["unique_to_1"], "unique_value");
}

}
