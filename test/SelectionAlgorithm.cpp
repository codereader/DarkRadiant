#include "RadiantTest.h"

#include "iselection.h"
#include "icommandsystem.h"
#include "selectionlib.h"

namespace test
{

TEST_F(RadiantTest, SelectItemsByModel)
{
    loadMap("select_items_by_model.map");

    auto staticMeshPath = "models/just_a_static_mesh.ase";
    auto md5MeshPath = "just_an_md5.md5mesh";

    GlobalSelectionSystem().setSelectedAll(false);
    ASSERT_TRUE(GlobalSelectionSystem().getSelectionInfo().totalCount == 0);

    // Select the two entities with the regular "model" "" spawnarg
    GlobalCommandSystem().executeCommand("SelectItemsByModel", { staticMeshPath });
    ASSERT_TRUE(GlobalSelectionSystem().getSelectionInfo().entityCount == 2);

    // Deselect the two entities with the regular "model" "" spawnarg
    GlobalCommandSystem().executeCommand("DeselectItemsByModel", { staticMeshPath });
    ASSERT_TRUE(GlobalSelectionSystem().getSelectionInfo().totalCount == 0);

    // Select the three entities that reference the md5 mesh through a model def
    GlobalCommandSystem().executeCommand("SelectItemsByModel", { md5MeshPath });
    ASSERT_TRUE(GlobalSelectionSystem().getSelectionInfo().entityCount == 3);

    GlobalCommandSystem().executeCommand("DeselectItemsByModel", { md5MeshPath });
    ASSERT_TRUE(GlobalSelectionSystem().getSelectionInfo().totalCount == 0);
}

}
