#include "RadiantTest.h"

#include <fstream>
#include "iundo.h"
#include "imap.h"
#include "imapformat.h"
#include "iradiant.h"
#include "iselectiongroup.h"
#include "ilightnode.h"
#include "icommandsystem.h"
#include "messages/FileSelectionRequest.h"
#include "algorithm/Scene.h"
#include "algorithm/XmlUtils.h"
#include "os/file.h"
#include <sigc++/connection.h>

namespace test
{

using MapLoadingTest = RadiantTest;
using MapSavingTest = RadiantTest;

TEST_F(MapLoadingTest, openMapWithEmptyStringAsksForPath)
{
    bool eventFired = false;

    // Subscribe to the event we expect to be fired
    auto msgSubscription = GlobalRadiantCore().getMessageBus().addListener(
        radiant::IMessage::Type::FileSelectionRequest,
        radiant::TypeListener<radiant::FileSelectionRequest>(
            [&](radiant::FileSelectionRequest& msg)
    {
        eventFired = true;
    }));

    // Calling OpenMap with no arguments will send a message to the UI
    GlobalCommandSystem().executeCommand("OpenMap");

    EXPECT_TRUE(eventFired) << "OpenMap didn't ask for a map file as expected";

    GlobalRadiantCore().getMessageBus().removeListener(msgSubscription);
}

void checkAltarScene()
{
    auto root = GlobalMapModule().getRoot();
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    
    // Check a specific spawnarg on worldspawn
    EXPECT_EQ(Node_getEntity(worldspawn)->getKeyValue("_color"), "0.286 0.408 0.259");

    // Check if all entities are present
    auto knownEntities = { "func_static_153", "func_static_154", "func_static_156",
        "func_static_155", "func_static_63", "func_static_66", "func_static_70",
        "func_static_164", "func_static_165", "light_torchflame_13", "religious_symbol_1" };
    
    for (auto entity : knownEntities)
    {
        EXPECT_TRUE(algorithm::getEntityByName(root, entity));
    }

    // Check number of patches
    auto isPatchDef3 = [](const scene::INodePtr& node) { return Node_isPatch(node) && Node_getIPatch(node)->subdivisionsFixed(); };
    auto isPatchDef2 = [](const scene::INodePtr& node) { return Node_isPatch(node) && !Node_getIPatch(node)->subdivisionsFixed(); };

    EXPECT_EQ(algorithm::getChildCount(worldspawn, isPatchDef3), 110); // 110 patchDef3
    EXPECT_EQ(algorithm::getChildCount(worldspawn, isPatchDef2), 16); // 16 patchDef2

    // Check number of brushes
    auto isBrush = [](const scene::INodePtr& node) { return Node_isBrush(node); };
    
    EXPECT_EQ(algorithm::getChildCount(root, isBrush), 37); // 37 brushes in total
    EXPECT_EQ(algorithm::getChildCount(worldspawn, isBrush), 21); // 21 worldspawn brushes
    EXPECT_EQ(algorithm::getChildCount(algorithm::getEntityByName(root, "func_static_66"), isBrush), 4); // 4 child brushes
    EXPECT_EQ(algorithm::getChildCount(algorithm::getEntityByName(root, "func_static_70"), isBrush), 4); // 4 child brushes
    EXPECT_EQ(algorithm::getChildCount(algorithm::getEntityByName(root, "func_static_164"), isBrush), 4); // 4 child brushes
    EXPECT_EQ(algorithm::getChildCount(algorithm::getEntityByName(root, "func_static_165"), isBrush), 4); // 4 child brushes

    // Check the lights
    auto isLight = [](const scene::INodePtr& node) { return Node_getLightNode(node) != nullptr; };
    EXPECT_EQ(algorithm::getChildCount(root, isLight), 1); // 1 light

    // Check a specific model
    auto religiousSymbol = Node_getEntity(algorithm::getEntityByName(root, "religious_symbol_1"));
    EXPECT_EQ(religiousSymbol->getKeyValue("classname"), "altar_moveable_loot_religious_symbol");
    EXPECT_EQ(religiousSymbol->getKeyValue("origin"), "-0.0448253 12.0322 -177");

    // Check layers
    EXPECT_TRUE(root->getLayerManager().getLayerID("Default") != -1);
    EXPECT_TRUE(root->getLayerManager().getLayerID("Windows") != -1);
    EXPECT_TRUE(root->getLayerManager().getLayerID("Lights") != -1);
    EXPECT_TRUE(root->getLayerManager().getLayerID("Ceiling") != -1);

    // Check map property
    EXPECT_EQ(root->getProperty("LastShaderClipboardMaterial"), "textures/tiles01");

    // Check selection group profile
    std::size_t numGroups = 0;
    std::multiset<std::size_t> groupCounts;
    root->getSelectionGroupManager().foreachSelectionGroup([&](const selection::ISelectionGroup& group)
    {
        ++numGroups;
        groupCounts.insert(group.size());
    });

    EXPECT_EQ(numGroups, 4);
    EXPECT_EQ(groupCounts.count(2), 1); // 1 group with 2 members
    EXPECT_EQ(groupCounts.count(3), 2); // 2 groups with 3 members
    EXPECT_EQ(groupCounts.count(12), 1); // 1 group with 12 members
}

TEST_F(MapLoadingTest, openMapFromAbsolutePath)
{
    // Generate an absolute path to a map in a temporary folder
    fs::path mapPath = _context.getTestResourcePath();
    mapPath /= "maps/altar.map";
    auto drFilePath = mapPath;
    drFilePath.replace_extension("darkradiant");

    // Copy to temp/
    fs::path temporaryMap = _context.getTemporaryDataPath();
    temporaryMap /= "temp_altar.map";
    auto temporaryDrFile = temporaryMap;
    temporaryDrFile.replace_extension("darkradiant");

    ASSERT_TRUE(fs::copy_file(mapPath, temporaryMap));
    ASSERT_TRUE(fs::copy_file(drFilePath, temporaryDrFile));

    GlobalCommandSystem().executeCommand("OpenMap", temporaryMap.string());

    // Check if the scene contains what we expect
    checkAltarScene();
}

TEST_F(MapLoadingTest, openMapFromModRelativePath)
{
    std::string modRelativePath = "maps/altar.map";

    // The map is located in maps/altar.map folder, check that it physically exists
    fs::path mapPath = _context.getTestResourcePath();
    mapPath /= modRelativePath;
    EXPECT_TRUE(os::fileOrDirExists(mapPath));
    
    GlobalCommandSystem().executeCommand("OpenMap", modRelativePath);

    // Check if the scene contains what we expect
    checkAltarScene();
}

TEST_F(MapLoadingTest, openMapFromMapsFolder)
{
    std::string mapName = "altar.map";

    // The map is located in maps/altar.map folder, check that it physically exists
    fs::path mapPath = _context.getTestResourcePath();
    mapPath /= "maps";
    mapPath /= mapName;
    EXPECT_TRUE(os::fileOrDirExists(mapPath));

    GlobalCommandSystem().executeCommand("OpenMap", mapName);

    // Check if the scene contains what we expect
    checkAltarScene();
}

TEST_F(MapLoadingTest, openMapFromModPak)
{
    std::string modRelativePath = "maps/altar_in_pk4.map";

    // The map is located in maps/ folder within the altar.pk4, check that it doesn't physically exist
    fs::path mapPath = _context.getTestResourcePath();
    mapPath /= modRelativePath;
    EXPECT_FALSE(os::fileOrDirExists(mapPath));

    GlobalCommandSystem().executeCommand("OpenMap", modRelativePath);

    // Check if the scene contains what we expect
    checkAltarScene();
}

TEST_F(MapLoadingTest, openNonExistentMap)
{
    std::string mapName = "idontexist.map";
    GlobalCommandSystem().executeCommand("OpenMap", mapName);

    // No worldspawn in this map, it should be empty
    EXPECT_FALSE(algorithm::getEntityByName(GlobalMapModule().getRoot(), "world"));
}

TEST_F(MapSavingTest, saveMapWithoutModification)
{
    loadMap("altar.map");
    checkAltarScene();

    bool mapSavedFired = false;
    sigc::connection conn = GlobalMapModule().signal_mapEvent().connect([&](IMap::MapEvent ev)
    {
        mapSavedFired |= ev == IMap::MapEvent::MapSaved;
    });

    EXPECT_FALSE(GlobalMapModule().isModified());

    GlobalCommandSystem().executeCommand("SaveMap");

    // SaveMap should trigger even though the map is not modified
    EXPECT_TRUE(mapSavedFired);

    conn.disconnect();
}

TEST_F(MapSavingTest, saveMapClearsModifiedFlag)
{
    // Create a copy of the original map, we're modifying it
    fs::path mapPath = _context.getTestResourcePath();
    mapPath /= "maps/altar.map";

    fs::path tempPath = _context.getTemporaryDataPath();
    tempPath /= "altar_modified_flag_test.map";
    
    // Copy both .map and .darkradiant file
    fs::copy(mapPath, tempPath);
    fs::copy(fs::path(mapPath).replace_extension("darkradiant"), fs::path(tempPath).replace_extension("darkradiant"));

    GlobalCommandSystem().executeCommand("OpenMap", tempPath.string());
    checkAltarScene();

    EXPECT_FALSE(GlobalMapModule().isModified());

    // Modify the worldspawn key values (in an Undoable transaction)
    algorithm::setWorldspawnKeyValue("dummykey", "dummyvalue");

    // This should mark the map as modified
    EXPECT_TRUE(GlobalMapModule().isModified());

    GlobalCommandSystem().executeCommand("SaveMap");

    // Modified flag should be cleared now
    EXPECT_FALSE(GlobalMapModule().isModified());
}

TEST_F(MapSavingTest, saveMapDoesntChangeMap)
{
    std::string modRelativePath = "maps/altar.map";

    // The map is located in maps/altar.map folder, check that it physically exists
    fs::path mapPath = _context.getTestResourcePath();
    mapPath /= modRelativePath;
    auto originalModificationDate = fs::last_write_time(mapPath);

    GlobalCommandSystem().executeCommand("OpenMap", modRelativePath);
    checkAltarScene();

    GlobalCommandSystem().executeCommand("SaveMap");

    // Check that the file got modified
    EXPECT_NE(fs::last_write_time(mapPath), originalModificationDate);

    // Load it again and check the scene
    GlobalCommandSystem().executeCommand("OpenMap", modRelativePath);
    checkAltarScene();
}

TEST_F(MapSavingTest, saveAs)
{
    std::string modRelativePath = "maps/altar.map";

    // The map is located in maps/altar.map folder, check that it physically exists
    fs::path mapPath = _context.getTestResourcePath();
    mapPath /= modRelativePath;
    auto originalModificationDate = fs::last_write_time(mapPath);

    GlobalCommandSystem().executeCommand("OpenMap", modRelativePath);
    checkAltarScene();

    // Select the format based on the extension
    auto format = GlobalMapFormatManager().getMapFormatForFilename(modRelativePath);
    fs::path tempPath = _context.getTemporaryDataPath();
    tempPath /= "altar_copy.map";

    // Subscribe to the event asking for the target path
    auto msgSubscription = GlobalRadiantCore().getMessageBus().addListener(
        radiant::IMessage::Type::FileSelectionRequest,
        radiant::TypeListener<radiant::FileSelectionRequest>(
            [&](radiant::FileSelectionRequest& msg)
    {
        msg.setHandled(true);
        msg.setResult(radiant::FileSelectionRequest::Result
        { 
            tempPath.string(), 
            format ->getMapFormatName()
        });
    }));

    EXPECT_FALSE(os::fileOrDirExists(tempPath));

    GlobalCommandSystem().executeCommand("SaveMapAs");

    // Check that the file got created
    EXPECT_TRUE(os::fileOrDirExists(tempPath));

    // The map path should have been changed
    EXPECT_EQ(GlobalMapModule().getMapName(), tempPath.string());

    // Load it again and check the scene
    GlobalCommandSystem().executeCommand("OpenMap", tempPath.string());
    checkAltarScene();

    GlobalRadiantCore().getMessageBus().removeListener(msgSubscription);
}

TEST_F(MapSavingTest, saveAsWithFormatWillContinueUsingThatFormat)
{
    std::string modRelativePath = "maps/altar.map";

    GlobalCommandSystem().executeCommand("OpenMap", modRelativePath);
    checkAltarScene();

    // Select the format based on the mapx extension
    fs::path tempPath = _context.getTemporaryDataPath();
    tempPath /= "altar_copy.mapx";

    auto format = GlobalMapFormatManager().getMapFormatForFilename(tempPath.string());
    EXPECT_EQ(format->getMapFormatName(), map::PORTABLE_MAP_FORMAT_NAME);

    // Subscribe to the event asking for the target path
    auto msgSubscription = GlobalRadiantCore().getMessageBus().addListener(
        radiant::IMessage::Type::FileSelectionRequest,
        radiant::TypeListener<radiant::FileSelectionRequest>(
            [&](radiant::FileSelectionRequest& msg)
    {
        msg.setHandled(true);
        msg.setResult(radiant::FileSelectionRequest::Result
            {
                tempPath.string(),
                format->getMapFormatName()
            });
    }));

    EXPECT_FALSE(os::fileOrDirExists(tempPath));

    GlobalCommandSystem().executeCommand("SaveMapAs");

    // Check that the file got created
    EXPECT_TRUE(os::fileOrDirExists(tempPath));
    algorithm::assertFileIsMapxFile(tempPath.string());

    // The map path should have been changed
    EXPECT_EQ(GlobalMapModule().getMapName(), tempPath.string());

    // Modify something in the map and save again
    algorithm::setWorldspawnKeyValue("dummykey222", "dummyvalue");

    GlobalCommandSystem().executeCommand("SaveMap");

    // Check that file, it should still be using the portable format
    algorithm::assertFileIsMapxFile(tempPath.string());

    GlobalRadiantCore().getMessageBus().removeListener(msgSubscription);
}

TEST_F(MapSavingTest, saveCopyAs)
{
    std::string modRelativePath = "maps/altar.map";

    GlobalCommandSystem().executeCommand("OpenMap", modRelativePath);
    checkAltarScene();

    EXPECT_EQ(GlobalMapModule().getMapName(), modRelativePath);

    // Select the format based on the extension
    auto format = GlobalMapFormatManager().getMapFormatForFilename(modRelativePath);
    fs::path tempPath = _context.getTemporaryDataPath();
    tempPath /= "altar_copy.map";

    // Subscribe to the event asking for the target path
    auto msgSubscription = GlobalRadiantCore().getMessageBus().addListener(
        radiant::IMessage::Type::FileSelectionRequest,
        radiant::TypeListener<radiant::FileSelectionRequest>(
            [&](radiant::FileSelectionRequest& msg)
    {
        msg.setHandled(true);
        msg.setResult(radiant::FileSelectionRequest::Result
            {
                tempPath.string(),
                format->getMapFormatName()
            });
    }));

    EXPECT_FALSE(os::fileOrDirExists(tempPath));

    GlobalCommandSystem().executeCommand("SaveMapCopyAs");

    // Check that the file got created
    EXPECT_TRUE(os::fileOrDirExists(tempPath));

    // The map path should NOT have been changed
    EXPECT_EQ(GlobalMapModule().getMapName(), modRelativePath);

    // Load the copy and verify the scene
    GlobalCommandSystem().executeCommand("OpenMap", tempPath.string());
    checkAltarScene();

    GlobalRadiantCore().getMessageBus().removeListener(msgSubscription);
}

TEST_F(MapSavingTest, saveCopyAsMapx)
{
    std::string modRelativePath = "maps/altar.map";

    GlobalCommandSystem().executeCommand("OpenMap", modRelativePath);
    checkAltarScene();

    EXPECT_EQ(GlobalMapModule().getMapName(), modRelativePath);

    // Select the format based on the mapx extension
    fs::path tempPath = _context.getTemporaryDataPath();
    tempPath /= "altar_copy.mapx";

    auto format = GlobalMapFormatManager().getMapFormatForFilename(tempPath.string());
    EXPECT_EQ(format->getMapFormatName(), map::PORTABLE_MAP_FORMAT_NAME);

    // Subscribe to the event asking for the target path
    auto msgSubscription = GlobalRadiantCore().getMessageBus().addListener(
        radiant::IMessage::Type::FileSelectionRequest,
        radiant::TypeListener<radiant::FileSelectionRequest>(
            [&](radiant::FileSelectionRequest& msg)
    {
        msg.setHandled(true);
        msg.setResult(radiant::FileSelectionRequest::Result
            {
                tempPath.string(),
                format->getMapFormatName()
            });
    }));

    EXPECT_FALSE(os::fileOrDirExists(tempPath));

    GlobalCommandSystem().executeCommand("SaveMapCopyAs");

    // Check that the file got created
    EXPECT_TRUE(os::fileOrDirExists(tempPath));

    // Check the output file
    algorithm::assertFileIsMapxFile(tempPath.string());

    // The map path should NOT have been changed
    EXPECT_EQ(GlobalMapModule().getMapName(), modRelativePath);

    // Load the portable format map and verify the scene
    GlobalCommandSystem().executeCommand("OpenMap", tempPath.string());
    checkAltarScene();

    EXPECT_EQ(GlobalMapModule().getMapName(), tempPath);

    GlobalRadiantCore().getMessageBus().removeListener(msgSubscription);
}

}
