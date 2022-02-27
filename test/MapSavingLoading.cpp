#include "RadiantTest.h"

#include <fstream>
#include "iundo.h"
#include "imap.h"
#include "imapformat.h"
#include "iautosaver.h"
#include "imapresource.h"
#include "ifilesystem.h"
#include "iradiant.h"
#include "iselectiongroup.h"
#include "ilightnode.h"
#include "icommandsystem.h"
#include "messages/ApplicationShutdownRequest.h"
#include "messages/FileSelectionRequest.h"
#include "messages/FileOverwriteConfirmation.h"
#include "messages/MapFileOperation.h"
#include "messages/FileSaveConfirmation.h"
#include "algorithm/Scene.h"
#include "algorithm/XmlUtils.h"
#include "algorithm/Primitives.h"
#include "os/file.h"
#include <sigc++/connection.h>
#include "testutil/FileSelectionHelper.h"
#include "registry/registry.h"

using namespace std::chrono_literals;

namespace test
{

namespace
{

class FileOverwriteHelper
{
private:
    std::size_t _msgSubscription;
    bool _shouldOverwrite;
    bool _messageReceived;

public:
    FileOverwriteHelper(bool shouldOverwrite) :
        _shouldOverwrite(shouldOverwrite),
        _messageReceived(false)
    {
        // Subscribe to the event asking for the target path
        _msgSubscription = GlobalRadiantCore().getMessageBus().addListener(
            radiant::IMessage::Type::FileOverwriteConfirmation,
            radiant::TypeListener<radiant::FileOverwriteConfirmation>(
                [this](radiant::FileOverwriteConfirmation& msg)
        {
            _messageReceived = true;
            msg.confirmOverwrite(_shouldOverwrite);
            msg.setHandled(true);
        }));
    }

    bool messageReceived() const
    {
        return _messageReceived;
    }

    ~FileOverwriteHelper()
    {
        GlobalRadiantCore().getMessageBus().removeListener(_msgSubscription);
    }
};

class FileSaveConfirmationHelper
{
private:
    std::size_t _msgSubscription;
    radiant::FileSaveConfirmation::Action _actionToTake;
    bool _messageReceived;

public:
    FileSaveConfirmationHelper(radiant::FileSaveConfirmation::Action actionToTake) :
        _actionToTake(actionToTake),
        _messageReceived(false)
    {
        // Subscribe to the event asking for the target path
        _msgSubscription = GlobalRadiantCore().getMessageBus().addListener(
            radiant::IMessage::Type::FileSaveConfirmation,
            radiant::TypeListener<radiant::FileSaveConfirmation>(
                [this](radiant::FileSaveConfirmation& msg)
        {
            _messageReceived = true;
            msg.setAction(_actionToTake);
            msg.setHandled(true);
        }));
    }

    bool messageReceived() const
    {
        return _messageReceived;
    }

    ~FileSaveConfirmationHelper()
    {
        GlobalRadiantCore().getMessageBus().removeListener(_msgSubscription);
    }
};

}

class MapFileTestBase :
    public RadiantTest
{
private:
    std::list<fs::path> _pathsToCleanupAfterTest;

protected:
    void preShutdown() override
    {
        for (const auto& path : _pathsToCleanupAfterTest)
        {
            fs::remove(path);
        }
    }

    // Creates a copy of the given map (including the .darkradiant file) in the temp data path
    // The copy will be removed in the TearDown() method
    fs::path createMapCopyInTempDataPath(const std::string& mapToCopy, const std::string& newFilename)
    {
        fs::path targetPath = _context.getTemporaryDataPath();
        targetPath /= newFilename;

        return createMapCopy(mapToCopy, targetPath);
    }

    // Creates a copy of the given map (including the .darkradiant file) in the mod-relative maps path
    // The copy will be removed in the TearDown() method
    fs::path createMapCopyInModMapsPath(const std::string& mapToCopy, const std::string& newFilename)
    {
        fs::path targetPath = _context.getTestProjectPath();
        targetPath /= "maps/";
        targetPath /= newFilename;

        return createMapCopy(mapToCopy, targetPath);
    }

private:
    fs::path createMapCopy(const std::string& mapToCopy, const fs::path& targetPath)
    {
        fs::path mapPath = _context.getTestProjectPath();
        mapPath /= "maps/";
        mapPath /= mapToCopy;

        fs::path targetInfoFilePath = fs::path(targetPath).replace_extension("darkradiant");

        _pathsToCleanupAfterTest.push_back(targetPath);
        _pathsToCleanupAfterTest.push_back(targetInfoFilePath);

        // Copy both .map and .darkradiant file
        fs::remove(targetPath);
        fs::remove(targetInfoFilePath);

        fs::copy(mapPath, targetPath);
        fs::copy(fs::path(mapPath).replace_extension("darkradiant"), targetInfoFilePath);

        return targetPath;
    }
};

using MapLoadingTest = MapFileTestBase;
using MapSavingTest = MapFileTestBase;

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

// Checks the main map info (brushes, entities, spawnargs), no layers or grouping
void checkAltarSceneGeometry(const scene::IMapRootNodePtr& root)
{
    auto worldspawn = algorithm::findWorldspawn(root);

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
}

void checkAltarSceneGeometry()
{
    checkAltarSceneGeometry(GlobalMapModule().getRoot());
}

// Check entire map including geometry, entities, layers, groups
void checkAltarScene(const scene::IMapRootNodePtr& root)
{
    checkAltarSceneGeometry(root);

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

void checkAltarScene()
{
    checkAltarScene(GlobalMapModule().getRoot());
}

TEST_F(MapLoadingTest, openMapFromAbsolutePath)
{
    // Generate an absolute path to a map in a temporary folder
    auto temporaryMap = createMapCopyInTempDataPath("altar.map", "temp_altar.map");

    GlobalCommandSystem().executeCommand("OpenMap", temporaryMap.string());

    // Check if the scene contains what we expect
    checkAltarScene();

    EXPECT_EQ(GlobalMapModule().getMapName(), temporaryMap.string());
}

TEST_F(MapLoadingTest, openMapFromModRelativePath)
{
    std::string modRelativePath = "maps/altar.map";

    // The map is located in maps/altar.map folder, check that it physically exists
    fs::path mapPath = _context.getTestProjectPath();
    mapPath /= modRelativePath;
    EXPECT_TRUE(os::fileOrDirExists(mapPath));

    GlobalCommandSystem().executeCommand("OpenMap", modRelativePath);

    // Check if the scene contains what we expect
    checkAltarScene();

    // Right now, even when opening from the mod folder, the map name is showing the absolute path
    //EXPECT_EQ(GlobalMapModule().getMapName(), "maps/" + mapName);
}

TEST_F(MapLoadingTest, openMapFromMapsFolder)
{
    std::string mapName = "altar.map";

    // The map is located in maps/altar.map folder, check that it physically exists
    fs::path mapPath = _context.getTestProjectPath();
    mapPath /= "maps";
    mapPath /= mapName;
    EXPECT_TRUE(os::fileOrDirExists(mapPath));

    GlobalCommandSystem().executeCommand("OpenMap", mapName);

    // Check if the scene contains what we expect
    checkAltarScene();

    // Right now, even when opening from the mod folder, the map name is showing the absolute path
    //EXPECT_EQ(GlobalMapModule().getMapName(), "maps/" + mapName);
}

TEST_F(MapLoadingTest, openMapFromModPak)
{
    std::string modRelativePath = "maps/altar_in_pk4.map";

    // The map is located in maps/ folder within the altar.pk4, check that it doesn't physically exist
    fs::path mapPath = _context.getTestProjectPath();
    mapPath /= modRelativePath;
    EXPECT_FALSE(os::fileOrDirExists(mapPath));

    GlobalCommandSystem().executeCommand("OpenMap", modRelativePath);

    // Check if the scene contains what we expect
    checkAltarScene();

    EXPECT_EQ(GlobalMapModule().getMapName(), modRelativePath);
}

TEST_F(MapLoadingTest, openMapFromArchive)
{
    auto pakPath = fs::path(_context.getTestResourcePath()) / "map_loading_test.pk4";
    std::string archiveRelativePath = "maps/altar_packed.map";

    GlobalCommandSystem().executeCommand("OpenMapFromArchive", pakPath.string(), archiveRelativePath);

    // Check if the scene contains what we expect
    checkAltarScene();
}

TEST_F(MapLoadingTest, openMapFromArchiveWithoutInfoFile)
{
    auto pakPath = fs::path(_context.getTestResourcePath()) / "map_loading_test.pk4";
    std::string archiveRelativePath = "maps/altar_packed_without_dr_file.map";

    GlobalCommandSystem().executeCommand("OpenMapFromArchive", pakPath.string(), archiveRelativePath);

    // Check if the scene contains what we expect, just the geometry since the map
    // is lacking its .darkradiant file
    checkAltarSceneGeometry();
}

TEST_F(MapLoadingTest, openNonExistentMap)
{
    std::string mapName = "idontexist.map";
    GlobalCommandSystem().executeCommand("OpenMap", mapName);

    // No worldspawn in this map, it should be empty
    EXPECT_FALSE(algorithm::getEntityByName(GlobalMapModule().getRoot(), "world"));

    EXPECT_EQ(GlobalMapModule().getMapName(), "unnamed.map");
}

TEST_F(MapLoadingTest, openWithInvalidPath)
{
    // Pass a directory name to it
    GlobalCommandSystem().executeCommand("OpenMap", _context.getTemporaryDataPath());

    // No worldspawn in this map, it should be empty
    EXPECT_FALSE(algorithm::getEntityByName(GlobalMapModule().getRoot(), "world"));

    EXPECT_EQ(GlobalMapModule().getMapName(), "unnamed.map");
}

TEST_F(MapLoadingTest, openWithInvalidPathInsideMod)
{
    // Pass a directory name to it
    GlobalCommandSystem().executeCommand("OpenMap", _context.getTestProjectPath());

    // No worldspawn in this map, it should be empty
    EXPECT_FALSE(algorithm::getEntityByName(GlobalMapModule().getRoot(), "world"));

    EXPECT_EQ(GlobalMapModule().getMapName(), "unnamed.map");
}

TEST_F(MapLoadingTest, openMapWithoutInfoFile)
{
    auto tempPath = createMapCopyInTempDataPath("altar.map", "altar_openMapWithoutInfoFile.map");

    fs::remove(fs::path(tempPath).replace_extension("darkradiant"));

    EXPECT_FALSE(os::fileOrDirExists(fs::path(tempPath).replace_extension("darkradiant")));

    GlobalCommandSystem().executeCommand("OpenMap", tempPath.string());

    checkAltarSceneGeometry();

    EXPECT_EQ(GlobalMapModule().getMapName(), tempPath.string());
}

TEST_F(MapLoadingTest, loadingCanBeCancelled)
{
    std::string mapName = "altar.map";
    std::string otherMap = "altar_loadingCanBeCancelled.map";
    auto tempPath = createMapCopyInTempDataPath(mapName, otherMap);

    // Load a valid map first
    GlobalCommandSystem().executeCommand("OpenMap", mapName);
    checkAltarScene();

    bool cancelIssued = false;

    // Subscribe to the map file operation progress event to cancel loading
    // Subscribe to the event we expect to be fired
    auto msgSubscription = GlobalRadiantCore().getMessageBus().addListener(
        radiant::IMessage::Type::MapFileOperation,
        radiant::TypeListener<map::FileOperation>(
            [&](map::FileOperation& msg)
    {
        if (msg.getOperationType() == map::FileOperation::Type::Import &&
            msg.getMessageType() == map::FileOperation::Started)
        {
            // set the flag before, since cancelOperation will be throwing an exception
            cancelIssued = true;
            msg.cancelOperation();
        }
    }));

    // Now load the other map and cancel the operation
    GlobalCommandSystem().executeCommand("OpenMap", tempPath.string());

    // Operation should have been actively cancelled
    EXPECT_TRUE(cancelIssued);

    // Map should be empty
    EXPECT_FALSE(algorithm::getEntityByName(GlobalMapModule().getRoot(), "world"));
    EXPECT_EQ(GlobalMapModule().getMapName(), "unnamed.map");

    GlobalRadiantCore().getMessageBus().removeListener(msgSubscription);
}

// Loading a map through MapResource::load without inserting the nodes into a scene,
// this should produce a valid scene too including the group information (which was a problem before)
TEST_F(MapLoadingTest, loadMapInResourceOnly)
{
    std::string modRelativePath = "maps/altar.map";

    auto resource = GlobalMapResourceManager().createFromPath(modRelativePath);
    EXPECT_TRUE(resource->load()) << "Test map not found: " << modRelativePath;

    checkAltarScene(resource->getRootNode());
}

TEST_F(MapLoadingTest, loadMapxInResourceOnly)
{
    // Save a mapx copy of the altar map
    GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/altar.map"));
    checkAltarScene();

    // Select the format based on the mapx extension
    fs::path tempPath = _context.getTemporaryDataPath();
    tempPath /= "altar_copy.mapx";

    auto format = GlobalMapFormatManager().getMapFormatForFilename(tempPath.string());
    EXPECT_EQ(format->getMapFormatName(), map::PORTABLE_MAP_FORMAT_NAME);

    FileSelectionHelper responder(tempPath.string(), format);
    EXPECT_FALSE(os::fileOrDirExists(tempPath));
    GlobalCommandSystem().executeCommand("SaveMapCopyAs");
    EXPECT_TRUE(os::fileOrDirExists(tempPath));

    // Now load the mapx copy and expect we have an intact scene
    auto resource = GlobalMapResourceManager().createFromPath(tempPath.string());
    EXPECT_TRUE(resource->load()) << "Copied map not found: " << tempPath.string();

    checkAltarScene(resource->getRootNode());
}

TEST_F(MapSavingTest, saveMapWithoutModification)
{
    auto tempPath = createMapCopyInTempDataPath("altar.map", "altar_saveMapWithoutModification.map");

    GlobalCommandSystem().executeCommand("OpenMap", tempPath.string());
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
    auto tempPath = createMapCopyInTempDataPath("altar.map", "altar_modified_flag_test.map");

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
    // Create a copy of the map file in the mod-relative maps/ folder
    fs::path mapPath = _context.getTestProjectPath();
    mapPath /= "maps/altar.map";

    // The map is located in maps/altar.map folder, check that it physically exists
    std::string modRelativePath = "maps/altar_saveMapDoesntChangeMap.map";
    fs::path copiedMap = _context.getTestProjectPath();
    copiedMap /= modRelativePath;

    fs::remove(copiedMap);
    fs::remove(fs::path(copiedMap).replace_extension("darkradiant"));

    fs::remove(copiedMap);
    fs::copy(mapPath, copiedMap);
    fs::copy(fs::path(mapPath).replace_extension("darkradiant"), fs::path(copiedMap).replace_extension("darkradiant"));

    auto originalModificationDate = fs::last_write_time(copiedMap);

    GlobalCommandSystem().executeCommand("OpenMap", modRelativePath);
    checkAltarScene();

    GlobalCommandSystem().executeCommand("SaveMap");

    // Check that the file got modified
    EXPECT_NE(fs::last_write_time(copiedMap), originalModificationDate);

    // Load it again and check the scene
    GlobalCommandSystem().executeCommand("OpenMap", modRelativePath);
    checkAltarScene();

    fs::remove(copiedMap);
    fs::remove(fs::path(copiedMap).replace_extension("bak"));
    fs::remove(fs::path(copiedMap).replace_extension("darkradiant"));
    fs::remove(fs::path(copiedMap).replace_extension("darkradiant").string() + ".bak");
}

TEST_F(MapSavingTest, saveMapCreatesInfoFile)
{
    // Ensure a worldspawn entity, this is enough
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();

    // Respond to the file selection request when saving
    fs::path tempPath = _context.getTemporaryDataPath();
    tempPath /= "just_a_worldspawn.map";
    auto infoFilePath = fs::path(tempPath).replace_extension("darkradiant");

    auto format = GlobalMapFormatManager().getMapFormatForFilename(tempPath.string());

    FileSelectionHelper responder(tempPath.string(), format);

    EXPECT_FALSE(os::fileOrDirExists(tempPath));
    EXPECT_FALSE(os::fileOrDirExists(infoFilePath));

    // Save the map
    GlobalCommandSystem().executeCommand("SaveMap");

    EXPECT_TRUE(os::fileOrDirExists(tempPath));
    EXPECT_TRUE(os::fileOrDirExists(infoFilePath));
}

namespace
{

// Shared algorithm to create a 6-brush map with layers
void doCheckSaveMapPreservesLayerInfo(const std::string& savePath, const map::MapFormatPtr& format)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();

    auto brush1 = algorithm::createCubicBrush(worldspawn, Vector3(400, 400, 400), "shader1");
    auto brush2 = algorithm::createCubicBrush(worldspawn, Vector3(500, 100, 200), "shader2");
    auto brush3 = algorithm::createCubicBrush(worldspawn, Vector3(300, 100, 200), "shader3");
    auto brush4 = algorithm::createCubicBrush(worldspawn, Vector3(300, 100, 700), "shader4");
    auto brush5 = algorithm::createCubicBrush(worldspawn, Vector3(300, 600, 700), "shader5");
    auto brush6 = algorithm::createCubicBrush(worldspawn, Vector3(300, 900, 700), "shader6");

    int layerId1 = GlobalMapModule().getRoot()->getLayerManager().createLayer("Layer1");
    int layerId2 = GlobalMapModule().getRoot()->getLayerManager().createLayer("Layer2");
    int layerId3 = GlobalMapModule().getRoot()->getLayerManager().createLayer("Layer3");
    int layerId4 = GlobalMapModule().getRoot()->getLayerManager().createLayer("Layer4");

    // Assign worldspawn to layers
    worldspawn->assignToLayers(scene::LayerList{ 0, layerId1, layerId2, layerId3, layerId4 });

    // Assign brushes to layers
    brush1->assignToLayers(scene::LayerList{ 0, layerId1 });
    brush2->assignToLayers(scene::LayerList{ layerId1, layerId2 });
    brush3->assignToLayers(scene::LayerList{ layerId3, layerId2 });
    brush4->assignToLayers(scene::LayerList{ 0, layerId4 });
    brush5->assignToLayers(scene::LayerList{ 0, layerId4 });
    brush6->assignToLayers(scene::LayerList{ layerId2, layerId4 });

    // Clear any referenes to the old scene
    worldspawn = brush1 = brush2 = brush3 = brush4 = brush5 = brush6 = scene::INodePtr();

    // Respond to the file selection request when saving
    FileSelectionHelper responder(savePath, format);

    // Save the map
    GlobalCommandSystem().executeCommand("SaveMap");

    // Clear the map, load from that file
    GlobalCommandSystem().executeCommand("OpenMap", savePath);

    // Refresh the node references from the new map
    worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    brush1 = algorithm::findFirstBrushWithMaterial(worldspawn, "shader1");
    brush2 = algorithm::findFirstBrushWithMaterial(worldspawn, "shader2");
    brush3 = algorithm::findFirstBrushWithMaterial(worldspawn, "shader3");
    brush4 = algorithm::findFirstBrushWithMaterial(worldspawn, "shader4");
    brush5 = algorithm::findFirstBrushWithMaterial(worldspawn, "shader5");
    brush6 = algorithm::findFirstBrushWithMaterial(worldspawn, "shader6");

    // Check the layers
    EXPECT_EQ(worldspawn->getLayers(), scene::LayerList({ 0, layerId1, layerId2, layerId3, layerId4 }));
    EXPECT_EQ(brush1->getLayers(), scene::LayerList({ 0, layerId1 }));
    EXPECT_EQ(brush2->getLayers(), scene::LayerList({ layerId1, layerId2 }));
    EXPECT_EQ(brush3->getLayers(), scene::LayerList({ layerId3, layerId2 }));
    EXPECT_EQ(brush4->getLayers(), scene::LayerList({ 0, layerId4 }));
    EXPECT_EQ(brush5->getLayers(), scene::LayerList({ 0, layerId4 }));
    EXPECT_EQ(brush6->getLayers(), scene::LayerList({ layerId2, layerId4 }));
}

}

TEST_F(MapSavingTest, saveMapPreservesLayerInfo)
{
    fs::path tempPath = _context.getTemporaryDataPath();
    tempPath /= "six_brushes_with_layers.map";
    auto format = GlobalMapFormatManager().getMapFormatForFilename(tempPath.string());

    doCheckSaveMapPreservesLayerInfo(tempPath.string(), format);
}

TEST_F(MapSavingTest, saveMapxPreservesLayerInfo)
{
    fs::path tempPath = _context.getTemporaryDataPath();
    tempPath /= "six_brushes_with_layers.mapx";
    auto format = GlobalMapFormatManager().getMapFormatForFilename(tempPath.string());

    doCheckSaveMapPreservesLayerInfo(tempPath.string(), format);
}

TEST_F(MapSavingTest, saveAs)
{
    std::string modRelativePath = "maps/altar.map";

    // The map is located in maps/altar.map folder, check that it physically exists
    fs::path mapPath = _context.getTestProjectPath();
    mapPath /= modRelativePath;
    EXPECT_TRUE(os::fileOrDirExists(mapPath));

    GlobalCommandSystem().executeCommand("OpenMap", modRelativePath);
    checkAltarScene();

    // Select the format based on the extension
    auto format = GlobalMapFormatManager().getMapFormatForFilename(modRelativePath);
    fs::path tempPath = _context.getTemporaryDataPath();
    tempPath /= "altar_copy.map";

    EXPECT_FALSE(os::fileOrDirExists(tempPath));

    // Respond to the event asking for the target path
    FileSelectionHelper responder(tempPath.string(), format);

    GlobalCommandSystem().executeCommand("SaveMapAs");

    // Check that the file got created
    EXPECT_TRUE(os::fileOrDirExists(tempPath));

    // The map path should have been changed
    EXPECT_EQ(GlobalMapModule().getMapName(), tempPath.string());

    // Load it again and check the scene
    GlobalCommandSystem().executeCommand("OpenMap", tempPath.string());
    checkAltarScene();
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

    FileSelectionHelper responder(tempPath.string(), format);

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

    FileSelectionHelper responder(tempPath.string(), format);

    EXPECT_FALSE(os::fileOrDirExists(tempPath));

    GlobalCommandSystem().executeCommand("SaveMapCopyAs");

    // Check that the file got created
    EXPECT_TRUE(os::fileOrDirExists(tempPath));

    // The map path should NOT have been changed
    EXPECT_EQ(GlobalMapModule().getMapName(), modRelativePath);

    // Load the copy and verify the scene
    GlobalCommandSystem().executeCommand("OpenMap", tempPath.string());
    checkAltarScene();
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

    FileSelectionHelper responder(tempPath.string(), format);

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
}

// Check that the overwriting an existing map file will create a backup set
TEST_F(MapSavingTest, saveMapCreatesBackup)
{
    auto mapPath = createMapCopyInTempDataPath("altar.map", "altar_saveMapCreatesBackup.map");

    auto originalSize = fs::file_size(mapPath);
    auto originalModDate = fs::last_write_time(mapPath);

    GlobalCommandSystem().executeCommand("OpenMap", mapPath.string());
    checkAltarScene();

    fs::path mapBackupPath = mapPath.replace_extension("bak");
    fs::path infoFileBackupPath = mapPath.replace_extension("darkradiant").string() + ".bak";

    EXPECT_FALSE(os::fileOrDirExists(mapBackupPath));
    EXPECT_FALSE(os::fileOrDirExists(infoFileBackupPath));

    GlobalCommandSystem().executeCommand("SaveMap");

    // Check that the target file got modified
    EXPECT_NE(fs::last_write_time(mapPath), originalModDate);

    // Check that the backup files exist now
    EXPECT_TRUE(os::fileOrDirExists(mapBackupPath));
    EXPECT_TRUE(os::fileOrDirExists(infoFileBackupPath));

    // The backup should have the write time of the original map
    EXPECT_EQ(fs::last_write_time(mapBackupPath), originalModDate);

    // Remove the backup at the end of this test
    fs::remove(mapBackupPath);
    fs::remove(infoFileBackupPath);
}

TEST_F(MapSavingTest, saveMapxCreatesBackup)
{
    std::string modRelativePath = "maps/altar.map";

    GlobalCommandSystem().executeCommand("OpenMap", modRelativePath);
    checkAltarScene();

    // Select the format based on the mapx extension
    auto mapxPath = fs::path(_context.getTemporaryDataPath()) / "altar_saveMapxCreatesBackup.mapx";

    auto format = GlobalMapFormatManager().getMapFormatForFilename(mapxPath.string());
    EXPECT_EQ(format->getMapFormatName(), map::PORTABLE_MAP_FORMAT_NAME);

    FileSelectionHelper responder(mapxPath.string(), format);

    GlobalCommandSystem().executeCommand("SaveMapAs");

    // Mapx file should be there, the backup should be missing
    EXPECT_TRUE(os::fileOrDirExists(mapxPath));
    EXPECT_FALSE(os::fileOrDirExists(fs::path(mapxPath).replace_extension("bak")));

    // Now we have a .mapx file lying around, overwrite it to create a backup
    GlobalCommandSystem().executeCommand("SaveMap");

    // Mapx backup should be there now
    EXPECT_TRUE(os::fileOrDirExists(fs::path(mapxPath).replace_extension("bak")));

    fs::remove(mapxPath);
    fs::remove(fs::path(mapxPath).replace_extension("bak"));
}

TEST_F(MapSavingTest, saveMapReplacesOldBackup)
{
    auto mapPath = createMapCopyInTempDataPath("altar.map", "altar_saveMapReplacesOldBackup.map");

    // Create two fake backup files
    fs::path mapBackupPath = fs::path(mapPath).replace_extension("bak");

    std::ofstream fakeBackup(mapBackupPath.string());
    fakeBackup << "123=map";
    fakeBackup.flush();
    fakeBackup.close();

    // Fake the mod time too, it needs to be different from the time
    // the above stream closes the file (otherwise the test might fail if it's fast enough)
    auto fakeTime = fs::last_write_time(mapBackupPath);
    fs::last_write_time(mapBackupPath, fakeTime + 1h);

    auto originalBackupSize = fs::file_size(mapBackupPath);
    auto originalBackupModTime = fs::last_write_time(mapBackupPath);

    fs::path infoFileBackupPath = fs::path(mapPath).replace_extension("darkradiant").string() + ".bak";

    std::ofstream fakeInfoBackup(infoFileBackupPath.string());
    fakeInfoBackup << "123=info";
    fakeInfoBackup.flush();
    fakeInfoBackup.close();

    fakeTime = fs::last_write_time(infoFileBackupPath);
    fs::last_write_time(infoFileBackupPath, fakeTime + 1h);

    auto originalInfoBackupSize = fs::file_size(infoFileBackupPath);
    auto originalInfoBackupModTime = fs::last_write_time(infoFileBackupPath);

    // Open the map, verify the scene
    GlobalCommandSystem().executeCommand("OpenMap", mapPath.string());
    checkAltarScene();

    // Overwrite the map, this should replace the backups
    GlobalCommandSystem().executeCommand("SaveMap");

    // Check that the backup files got modified
    EXPECT_NE(fs::last_write_time(mapBackupPath), originalBackupModTime);
    EXPECT_NE(fs::last_write_time(infoFileBackupPath), originalInfoBackupModTime);

    EXPECT_NE(fs::file_size(mapBackupPath), originalBackupSize);
    EXPECT_NE(fs::file_size(infoFileBackupPath), originalInfoBackupSize);
}

TEST_F(MapSavingTest, saveMapOpenedInModRelativePath)
{
    std::string mapFileName = "altar_saveMapOpenedWithModRelativePath.map";
    auto tempPath = createMapCopyInModMapsPath("altar.map", mapFileName);

    GlobalCommandSystem().executeCommand("OpenMap", "maps/" + mapFileName);
    checkAltarScene();

    auto originalModDate = fs::last_write_time(tempPath);

    GlobalCommandSystem().executeCommand("SaveMap");

    // File should have been modified
    EXPECT_NE(fs::last_write_time(tempPath), originalModDate);

    // Remove the backups
    fs::remove(tempPath.replace_extension("bak"));
    fs::remove(tempPath.replace_extension("darkradiant").string() + ".bak");
}

TEST_F(MapSavingTest, saveMapOpenedInAbsolutePath)
{
    std::string mapFileName = "altar_saveMapOpenedWithAbsolutePath.map";
    auto tempPath = createMapCopyInTempDataPath("altar.map", mapFileName);

    GlobalCommandSystem().executeCommand("OpenMap", tempPath.string());
    checkAltarScene();

    auto originalModDate = fs::last_write_time(tempPath);

    GlobalCommandSystem().executeCommand("SaveMap");

    // File should have been modified
    EXPECT_NE(fs::last_write_time(tempPath), originalModDate);

    // Remove the backups
    fs::remove(tempPath.replace_extension("bak"));
    fs::remove(tempPath.replace_extension("darkradiant").string() + ".bak");
}

TEST_F(MapSavingTest, saveArchivedMapWillAskForFilename)
{
    // A map that has been loaded from an archive file is not writeable
    // the map saving algorithm should detect this and ask for a new file location
    auto pakPath = fs::path(_context.getTestResourcePath()) / "map_loading_test.pk4";
    std::string archiveRelativePath = "maps/altar_packed.map";

    GlobalCommandSystem().executeCommand("OpenMapFromArchive", pakPath.string(), archiveRelativePath);
    checkAltarScene();

    bool eventFired = false;
    fs::path outputPath = _context.getTemporaryDataPath();
    outputPath /= "altar_packed_copy.map";
    auto format = GlobalMapFormatManager().getMapFormatForFilename(outputPath.string());

    // Subscribe to the event asking for the target path
    auto msgSubscription = GlobalRadiantCore().getMessageBus().addListener(
        radiant::IMessage::Type::FileSelectionRequest,
        radiant::TypeListener<radiant::FileSelectionRequest>(
            [&](radiant::FileSelectionRequest& msg)
    {
        eventFired = true;
        msg.setHandled(true);
        msg.setResult(radiant::FileSelectionRequest::Result
        {
            outputPath.string(),
            format->getMapFormatName()
        });
    }));

    // This should ask the user for a file location, i.e. fire the above event
    GlobalCommandSystem().executeCommand("SaveMap");

    EXPECT_TRUE(eventFired) << "Radiant didn't ask for a new filename when saving an archived map";

    eventFired = false;

    // Save again, this should no longer ask for a location
    GlobalCommandSystem().executeCommand("SaveMap");

    EXPECT_FALSE(eventFired);

    GlobalRadiantCore().getMessageBus().removeListener(msgSubscription);
}

TEST_F(MapSavingTest, savingUnnamedMapDoesntWarnAboutOverwrite)
{
    EXPECT_TRUE(GlobalMapModule().isUnnamed());

    fs::path outputPath = _context.getTemporaryDataPath();
    outputPath /= "whatevername.mapx";
    auto format = GlobalMapFormatManager().getMapFormatForFilename(outputPath.string());

    EXPECT_FALSE(os::fileOrDirExists(outputPath));

    // Save to a temporary path
    FileSelectionHelper saveHelper(outputPath.string(), format);
    FileOverwriteHelper overwriteHelper(false); // don't overwrite

    // This should ask the user for a file location, i.e. fire the above event
    // but shouldn't warn about for overwriting (file doesn't exist)
    GlobalCommandSystem().executeCommand("SaveMap");

    // File should have been created without overwrite event
    EXPECT_TRUE(os::fileOrDirExists(outputPath));
    EXPECT_FALSE(overwriteHelper.messageReceived());
}

TEST_F(MapSavingTest, savingUnnamedMapDoesntWarnAboutOverwriteWhenFileExists)
{
    EXPECT_TRUE(GlobalMapModule().isUnnamed());

    fs::path outputPath = _context.getTemporaryDataPath();
    outputPath /= "whatevername.mapx";
    auto format = GlobalMapFormatManager().getMapFormatForFilename(outputPath.string());

    std::ofstream stream(outputPath);
    stream << "Test";
    stream.flush();
    stream.close();

    EXPECT_TRUE(os::fileOrDirExists(outputPath));

    // Save to a temporary path
    FileSelectionHelper saveHelper(outputPath.string(), format);
    FileOverwriteHelper overwriteHelper(false); // don't overwrite

    // This should ask the user for a file location, i.e. fire the above event
    // but shouldn't warn about for overwriting (file doesn't exist)
    GlobalCommandSystem().executeCommand("SaveMap");

    // File should have been created without overwrite event
    algorithm::assertFileIsMapxFile(outputPath.string());
    EXPECT_FALSE(overwriteHelper.messageReceived());

    // Remove the backup file
    fs::remove(outputPath.replace_extension("bak"));
}

TEST_F(MapSavingTest, savingOpenedMapFileDoesntWarnAboutOverwrite)
{
    std::string mapFileName = "altar_savingOpenedMapFileDoesntAskForOverwrite.map";
    auto tempPath = createMapCopyInTempDataPath("altar.map", mapFileName);

    GlobalCommandSystem().executeCommand("OpenMap", tempPath.string());
    checkAltarScene();

    auto originalModDate = fs::last_write_time(tempPath);

    // Monitor the event
    FileOverwriteHelper overwriteHelper(false); // don't overwrite

    GlobalCommandSystem().executeCommand("SaveMap");

    // File should have been modified, no asking for overwrite
    EXPECT_NE(fs::last_write_time(tempPath), originalModDate);
    EXPECT_FALSE(overwriteHelper.messageReceived());

    // Remove the backups
    fs::remove(tempPath.replace_extension("bak"));
    fs::remove(tempPath.replace_extension("darkradiant").string() + ".bak");
}

TEST_F(MapSavingTest, saveAsDoesntWarnAboutOverwrite)
{
    std::string modRelativePath = "maps/altar.map";

    GlobalCommandSystem().executeCommand("OpenMap", modRelativePath);
    checkAltarScene();

    // Select the format based on the extension
    fs::path tempPath = _context.getTemporaryDataPath();
    tempPath /= "altar_saveAsDoesntWarnAboutOverwrite.map";
    auto format = GlobalMapFormatManager().getMapFormatForFilename(tempPath.string());

    EXPECT_FALSE(os::fileOrDirExists(tempPath));

    // Respond to the event asking for the target path
    FileSelectionHelper responder(tempPath.string(), format);
    FileOverwriteHelper overwriteHelper(false); // don't overwrite

    GlobalCommandSystem().executeCommand("SaveMapAs");

    // Check that the file got created
    EXPECT_TRUE(os::fileOrDirExists(tempPath));
    EXPECT_FALSE(overwriteHelper.messageReceived());
}

TEST_F(MapSavingTest, saveAsDoesntWarnAboutOverwriteWhenFileExists)
{
    std::string modRelativePath = "maps/altar.map";

    GlobalCommandSystem().executeCommand("OpenMap", modRelativePath);
    checkAltarScene();

    // Select the format based on the extension
    fs::path tempPath = _context.getTemporaryDataPath();
    tempPath /= "altar_saveAsDoesntWarnAboutOverwrite.map";
    auto format = GlobalMapFormatManager().getMapFormatForFilename(tempPath.string());

    EXPECT_FALSE(os::fileOrDirExists(tempPath));

    // Create the file with some dummy content
    std::ofstream stream(tempPath);
    stream << "Test";
    stream.flush();
    stream.close();
    // Set modification time back by 5s
    auto fakeModDate = fs::last_write_time(tempPath) - 5s;
    fs::last_write_time(tempPath, fakeModDate);

    EXPECT_TRUE(os::fileOrDirExists(tempPath));

    // Respond to the event asking for the target path
    FileSelectionHelper responder(tempPath.string(), format);
    FileOverwriteHelper overwriteHelper(false); // don't overwrite

    GlobalCommandSystem().executeCommand("SaveMapAs");

    // File date must have been changed
    EXPECT_NE(fs::last_write_time(tempPath), fakeModDate);

    EXPECT_FALSE(overwriteHelper.messageReceived());
}

TEST_F(MapSavingTest, saveWarnsAboutOverwrite)
{
    std::string mapFileName = "altar_saveWarnsAboutOverwrite.map";
    auto tempPath = createMapCopyInTempDataPath("altar.map", mapFileName);

    // Set the modification time back a bit, the unit test might be moving fast
    auto fakeModDate = fs::last_write_time(tempPath) - 5s;
    fs::last_write_time(tempPath, fakeModDate);

    GlobalCommandSystem().executeCommand("OpenMap", tempPath.string());
    checkAltarScene();

    // Modify the file by replacing its contents
    std::ofstream stream(tempPath);
    stream << "Test";
    stream.flush();
    stream.close();

    // File date must have been changed
    EXPECT_NE(fs::last_write_time(tempPath), fakeModDate);

    // Monitor the warn about overwrite event
    FileOverwriteHelper overwriteHelper(true); // confirm overwrite

    GlobalCommandSystem().executeCommand("SaveMap");

    // Check that the event was fired
    EXPECT_TRUE(overwriteHelper.messageReceived());

    // File should have been written
    GlobalCommandSystem().executeCommand("OpenMap", tempPath.string());
    checkAltarScene();
}

TEST_F(MapSavingTest, saveWarnsAboutOverwriteAndUserCancels)
{
    std::string mapFileName = "altar_saveWarnsAboutOverwriteAndUserCancels.map";
    auto tempPath = createMapCopyInTempDataPath("altar.map", mapFileName);

    // Set the modification time back a bit, the unit test might be moving fast
    auto fakeModDate = fs::last_write_time(tempPath) - 5s;
    fs::last_write_time(tempPath, fakeModDate);

    GlobalCommandSystem().executeCommand("OpenMap", tempPath.string());
    checkAltarScene();

    // Modify the file by replacing its contents
    constexpr const char* tempContents = "Test345";
    std::ofstream stream(tempPath);
    stream << tempContents;
    stream.flush();
    stream.close();

    // File date must have been changed
    EXPECT_NE(fs::last_write_time(tempPath), fakeModDate);

    // Monitor the warn about overwrite event
    FileOverwriteHelper overwriteHelper(false); // block overwrite

    GlobalCommandSystem().executeCommand("SaveMap");

    // Check that the event was fired
    EXPECT_TRUE(overwriteHelper.messageReceived());

    // File should not have been replaced
    std::ifstream writtenFile(tempPath);
    std::stringstream contents;
    contents << writtenFile.rdbuf();

    EXPECT_EQ(contents.str(), tempContents);
}

// #5729: Autosaver overwrites the stored filename that has been previously used for "Save Copy As"
TEST_F(MapSavingTest, AutoSaverDoesntChangeSaveCopyAsFilename)
{
    std::string modRelativePath = "maps/altar.map";

    GlobalCommandSystem().executeCommand("OpenMap", modRelativePath);
    checkAltarScene();

    std::string pathThatWasSentAsDefaultPath;

    fs::path tempPath = _context.getTemporaryDataPath();
    tempPath /= "altar_copy.map";

    // Subscribe to the event asking for the target path
    auto msgSubscription = GlobalRadiantCore().getMessageBus().addListener(
        radiant::IMessage::Type::FileSelectionRequest,
        radiant::TypeListener<radiant::FileSelectionRequest>(
            [&](radiant::FileSelectionRequest& msg)
    {
        pathThatWasSentAsDefaultPath = msg.getDefaultFile();

        auto format = GlobalMapFormatManager().getMapFormatForFilename(tempPath.string());

        msg.setHandled(true);
        msg.setResult(radiant::FileSelectionRequest::Result{ tempPath.string(), format->getMapFormatName() });
    }));

    EXPECT_FALSE(os::fileOrDirExists(tempPath));

    // This will ask for a file name, and we respond with the "altar_copy.map"
    GlobalCommandSystem().executeCommand("SaveMapCopyAs");
    GlobalCommandSystem().executeCommand("SaveMapCopyAs");

    // Check that the file got created and remove it again
    EXPECT_TRUE(os::fileOrDirExists(tempPath));
    if (fs::exists(tempPath)) fs::remove(tempPath);
    EXPECT_FALSE(os::fileOrDirExists(tempPath));

    // Now trigger an autosave
    GlobalAutoSaver().performAutosave();

    // This will (again) ask for a file name, now we check what map file name it remembered and 
    // sent to the request handler as default file name
    GlobalCommandSystem().executeCommand("SaveMapCopyAs");

    EXPECT_TRUE(os::fileOrDirExists(tempPath));
    if (fs::exists(tempPath)) fs::remove(tempPath);

    EXPECT_EQ(pathThatWasSentAsDefaultPath, tempPath.string()) << "Autosaver overwrote the stored file name for 'Save Copy As'";

    GlobalRadiantCore().getMessageBus().removeListener(msgSubscription);
}

TEST_F(MapSavingTest, AutoSaveSnapshotsSupportRelativePaths)
{
    std::string modRelativePath = "maps/altar.map";
    GlobalCommandSystem().executeCommand("OpenMap", modRelativePath);
    checkAltarScene();

    // Set this to a relative path
    registry::setValue(map::RKEY_AUTOSAVE_SNAPSHOTS_ENABLED, true);
    registry::setValue(map::RKEY_AUTOSAVE_SNAPSHOTS_FOLDER, "customsnapshots/");

    // We expect the file to end up here
    std::string expectedSnapshotPath = "maps/customsnapshots/altar.0.map";

    EXPECT_FALSE(GlobalFileSystem().openTextFile(expectedSnapshotPath)) << "Snapshot already exists in " << expectedSnapshotPath;

    // Trigger an auto save now
    GlobalAutoSaver().performAutosave();

    EXPECT_TRUE(GlobalFileSystem().openTextFile(expectedSnapshotPath)) << "Snapshot should now exist in " << expectedSnapshotPath;
    
    // Load and confirm the saved scene
    GlobalCommandSystem().executeCommand("OpenMap", expectedSnapshotPath);
    checkAltarScene();

    auto fullPath = GlobalFileSystem().findFile(expectedSnapshotPath) + expectedSnapshotPath;
    EXPECT_NE(fullPath, "") << "Failed to resolve absolute file path of " << expectedSnapshotPath;

    if (!fullPath.empty())
    {
        fs::remove(os::replaceExtension(fullPath, "darkradiant"));
        fs::remove(fullPath);
    }
}

TEST_F(MapSavingTest, AutoSaveSnapshotsSupportAbsolutePaths)
{
    std::string modRelativePath = "maps/altar.map";
    GlobalCommandSystem().executeCommand("OpenMap", modRelativePath);
    checkAltarScene();

    // Set this to an absolute path
    auto snapshotFolder = _context.getTemporaryDataPath() + "customsnapshots/";
    registry::setValue(map::RKEY_AUTOSAVE_SNAPSHOTS_ENABLED, true);
    registry::setValue(map::RKEY_AUTOSAVE_SNAPSHOTS_FOLDER, snapshotFolder);

    // We expect the file to end up there
    std::string expectedSnapshotPath = snapshotFolder + "altar.0.map";

    EXPECT_FALSE(GlobalFileSystem().openTextFileInAbsolutePath(expectedSnapshotPath)) << "Snapshot already exists in " << expectedSnapshotPath;

    // Trigger an auto save now
    GlobalAutoSaver().performAutosave();

    EXPECT_TRUE(GlobalFileSystem().openTextFileInAbsolutePath(expectedSnapshotPath)) << "Snapshot should now exist in " << expectedSnapshotPath;

    // Load and confirm the saved scene
    GlobalCommandSystem().executeCommand("OpenMap", expectedSnapshotPath);
    checkAltarScene();

    fs::remove(os::replaceExtension(expectedSnapshotPath, "darkradiant"));
    fs::remove(expectedSnapshotPath);
}

namespace
{

void checkBehaviourWithoutUnsavedChanges(std::function<void()> action)
{
    std::string modRelativePath = "maps/altar.map";
    GlobalCommandSystem().executeCommand("OpenMap", modRelativePath);

    FileSaveConfirmationHelper helper(radiant::FileSaveConfirmation::Action::SaveChanges);

    // The map has not been changed
    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map was marked as modified after loading";

    // Performing the action will not ask for save, since it hasn't been changed
    action();

    EXPECT_FALSE(helper.messageReceived()) << "Map shouldn't have asked for save";
    EXPECT_FALSE(GlobalMapModule().isModified()) << "New Map should be unmodified";
}

void checkBehaviourDiscardingUnsavedChanges(const std::string& testProjectPath, std::function<void()> action, bool expectUnmodifiedMapAfterAction = true)
{
    std::string modRelativePath = "maps/altar.map";
    GlobalCommandSystem().executeCommand("OpenMap", modRelativePath);

    fs::path mapPath = testProjectPath + modRelativePath;
    auto sizeBefore = fs::file_size(mapPath);

    // Create a brush to mark the map as modified
    algorithm::createCubicBrush(GlobalMapModule().findOrInsertWorldspawn());
    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map was not marked as modified after brush creation";

    // Discard the changes
    FileSaveConfirmationHelper helper(radiant::FileSaveConfirmation::Action::DiscardChanges);

    // Performing the action must ask for save, since the file has been changed
    action();

    EXPECT_TRUE(helper.messageReceived()) << "Map must ask for save";
    EXPECT_EQ(fs::file_size(mapPath), sizeBefore) << "File size has changed after discarding";

    // In some scenario like app shutdown the map modified flag is not cleared
    if (expectUnmodifiedMapAfterAction)
    {
        EXPECT_FALSE(GlobalMapModule().isModified()) << "Map should be unmodified again";
    }
}

void checkBehaviourSavingUnsavedChanges(const std::string& fullMapPath, std::function<void()> action)
{
    GlobalCommandSystem().executeCommand("OpenMap", fullMapPath);

    auto sizeBefore = fs::file_size(fullMapPath);

    // Create a brush to mark the map as modified
    algorithm::createCubicBrush(GlobalMapModule().findOrInsertWorldspawn());
    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map was not marked as modified after brush creation";

    // Save the changes
    FileSaveConfirmationHelper helper(radiant::FileSaveConfirmation::Action::SaveChanges);

    // Performing the action must ask for save, since the file has been changed
    action();

    EXPECT_TRUE(helper.messageReceived()) << "Map must ask for save";
    EXPECT_NE(fs::file_size(fullMapPath), sizeBefore) << "File size must have changed after saving";
    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map should be unmodified again";
}

void checkBehaviourCancellingMapChange(const std::string& fullMapPath, std::function<void()> action)
{
    GlobalCommandSystem().executeCommand("OpenMap", fullMapPath);

    auto sizeBefore = fs::file_size(fullMapPath);

    // Create a brush to mark the map as modified
    algorithm::createCubicBrush(GlobalMapModule().findOrInsertWorldspawn());
    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map was not marked as modified after brush creation";

    // Discard the changes
    FileSaveConfirmationHelper helper(radiant::FileSaveConfirmation::Action::Cancel);

    // Creating a new map must ask for save, since the file has been changed
    GlobalCommandSystem().executeCommand("NewMap");

    // We should still have the map loaded
    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map was not marked as modified after brush creation";

    EXPECT_TRUE(helper.messageReceived()) << "Map must ask for save";
    EXPECT_EQ(fs::file_size(fullMapPath), sizeBefore) << "File size has changed after discarding";
}

void openMapFromArchive(const radiant::TestContext& context)
{
    auto pakPath = fs::path(context.getTestResourcePath()) / "map_loading_test.pk4";
    std::string archiveRelativePath = "maps/altar_packed.map";

    GlobalCommandSystem().executeCommand("OpenMapFromArchive", pakPath.string(), archiveRelativePath);
}

void sendShutdownRequest(bool expectDenial)
{
    // Send the shutdown request, the Map Module should be receiving this
    radiant::ApplicationShutdownRequest request;
    GlobalRadiantCore().getMessageBus().sendMessage(request);

    EXPECT_EQ(expectDenial, request.isDenied()) << "Shutdown Request denial flag has unexpected value";
}

}

TEST_F(MapSavingTest, NewMapWithoutUnsavedChanges)
{
    checkBehaviourWithoutUnsavedChanges([]()
    {
        // Creating a new map will not ask for save, since it hasn't been changed
        GlobalCommandSystem().executeCommand("NewMap");
    });
}

TEST_F(MapSavingTest, OpenMapWithoutUnsavedChanges)
{
    checkBehaviourWithoutUnsavedChanges([]()
    {
        // Opening a new map will not ask for save, since it hasn't been changed
        GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/csg_merge.map"));
    });
}

TEST_F(MapSavingTest, OpenMapFromArchiveWithoutUnsavedChanges)
{
    checkBehaviourWithoutUnsavedChanges([&]()
    {
        openMapFromArchive(_context);
    });
}

TEST_F(MapSavingTest, RadiantShutdownWithoutUnsavedChanges)
{
    checkBehaviourWithoutUnsavedChanges([]()
    {
        sendShutdownRequest(false); // no denial
    });
}

TEST_F(MapSavingTest, NewMapDiscardingUnsavedChanges)
{
    checkBehaviourDiscardingUnsavedChanges(_context.getTestProjectPath(), []()
    {
        // Creating a new map must ask for save, since the file has been changed
        GlobalCommandSystem().executeCommand("NewMap");
    });
}

TEST_F(MapSavingTest, OpenMapDiscardingUnsavedChanges)
{
    checkBehaviourDiscardingUnsavedChanges(_context.getTestProjectPath(), []()
    {
        // Opening a new map must ask for save, since the file has been changed
        GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/csg_merge.map"));
    });
}

TEST_F(MapSavingTest, OpenMapFromArchiveDiscardingUnsavedChanges)
{
    checkBehaviourDiscardingUnsavedChanges(_context.getTestProjectPath(), [&]()
    {
        openMapFromArchive(_context);
    });
}

TEST_F(MapSavingTest, RadiantShutdownDiscardingUnsavedChanges)
{
    // Pass false to expectUnmodifiedMapAfterAction => discarding the map on shutdown will not clear the modified flag
    checkBehaviourDiscardingUnsavedChanges(_context.getTestProjectPath(), []()
    {
        sendShutdownRequest(false); // no denial
    }, false);
}

TEST_F(MapSavingTest, NewMapSavingChanges)
{
    auto mapPath = createMapCopyInTempDataPath("altar.map", "altar_NewMapSavingChanges.map");

    checkBehaviourSavingUnsavedChanges(mapPath.string(), []()
    {
        // Creating a new map must ask for save, since the file has been changed
        GlobalCommandSystem().executeCommand("NewMap");
    });
}

TEST_F(MapSavingTest, OpenMapSavingChanges)
{
    auto mapPath = createMapCopyInTempDataPath("altar.map", "altar_NewMapSavingChanges.map");

    checkBehaviourSavingUnsavedChanges(mapPath.string(), []()
    {
        // Opening a new map must ask for save, since the file has been changed
        GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/csg_merge.map"));
    });
}

TEST_F(MapSavingTest, OpenMapFromArchiveSavingChanges)
{
    auto mapPath = createMapCopyInTempDataPath("altar.map", "altar_NewMapSavingChanges.map");

    checkBehaviourSavingUnsavedChanges(mapPath.string(), [&]()
    {
        openMapFromArchive(_context);
    });
}

TEST_F(MapSavingTest, RadiantShutdownSavingChanges)
{
    auto mapPath = createMapCopyInTempDataPath("altar.map", "altar_NewMapSavingChanges.map");

    checkBehaviourSavingUnsavedChanges(mapPath.string(), []()
    {
        sendShutdownRequest(false); // no denial
    });
}

TEST_F(MapSavingTest, NewMapCancelWithUnsavedChanges)
{
    auto mapPath = createMapCopyInTempDataPath("altar.map", "altar_NewMapCancelWithUnsavedChanges.map");

    checkBehaviourCancellingMapChange(mapPath.string(), []()
    {
        // Creating a new map must ask for save, since the file has been changed
        GlobalCommandSystem().executeCommand("NewMap");
    });
}

TEST_F(MapSavingTest, OpenMapCancelWithUnsavedChanges)
{
    auto mapPath = createMapCopyInTempDataPath("altar.map", "altar_NewMapCancelWithUnsavedChanges.map");

    checkBehaviourCancellingMapChange(mapPath.string(), []()
    {
        // Opening a new map must ask for save, since the file has been changed
        GlobalCommandSystem().executeCommand("OpenMap", cmd::Argument("maps/csg_merge.map"));
    });
}

TEST_F(MapSavingTest, OpenMapFromArchiveCancelWithUnsavedChanges)
{
    auto mapPath = createMapCopyInTempDataPath("altar.map", "altar_NewMapCancelWithUnsavedChanges.map");

    checkBehaviourCancellingMapChange(mapPath.string(), [&]()
    {
        openMapFromArchive(_context);
    });
}

TEST_F(MapSavingTest, RadiantShutdownCancelWithUnsavedChanges)
{
    auto mapPath = createMapCopyInTempDataPath("altar.map", "altar_NewMapCancelWithUnsavedChanges.map");

    checkBehaviourCancellingMapChange(mapPath.string(), []()
    {
        sendShutdownRequest(true); // request should be denied
    });
}

}
