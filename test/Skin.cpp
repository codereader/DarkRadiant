#include "RadiantTest.h"

#include "modelskin.h"
#include "scenelib.h"
#include "algorithm/Entity.h"
#include "algorithm/Scene.h"
#include "testutil/TemporaryFile.h"

namespace test
{

using ModelSkinTest = RadiantTest;

TEST_F(ModelSkinTest, FindSkins)
{
    // All of these declarations need to be parsed and present
    auto tileSkin = GlobalModelSkinCache().findSkin("tile_skin");
    EXPECT_EQ(tileSkin->getDeclName(), "tile_skin");
    EXPECT_EQ(tileSkin->getDeclFilePath(), "skins/test_skins.skin");
    EXPECT_EQ(tileSkin->getRemap("textures/atest/a"), "textures/numbers/10");

    auto separatedTileSkin = GlobalModelSkinCache().findSkin("separated_tile_skin");
    EXPECT_EQ(separatedTileSkin->getDeclName(), "separated_tile_skin");
    EXPECT_EQ(separatedTileSkin->getDeclFilePath(), "skins/test_skins.skin");
    EXPECT_EQ(separatedTileSkin->getRemap("material"), "textures/numbers/11");

    auto skinWithStrangeCasing = GlobalModelSkinCache().findSkin("skin_with_strange_casing");
    EXPECT_EQ(skinWithStrangeCasing->getDeclName(), "skin_with_strange_casing");
    EXPECT_EQ(skinWithStrangeCasing->getDeclFilePath(), "skins/test_skins.skin");
    EXPECT_EQ(skinWithStrangeCasing->getRemap("material"), "textures/numbers/11");

    auto ivyOnesided = GlobalModelSkinCache().findSkin("ivy_onesided");
    EXPECT_EQ(ivyOnesided->getDeclName(), "ivy_onesided");
    EXPECT_EQ(ivyOnesided->getDeclFilePath(), "skins/selection_test.skin");
    EXPECT_EQ(ivyOnesided->getRemap("textures/darkmod/decals/vegetation/ivy_mixed_pieces"), 
        "textures/darkmod/decals/vegetation/ivy_mixed_pieces_onesided");
}

TEST_F(ModelSkinTest, FindSkinsIsCaseInsensitive)
{
    // This is a different spelling than the one used in the decl file
    auto tileSkin = GlobalModelSkinCache().findSkin("tILE_skiN");

    EXPECT_NE(tileSkin->getDeclName(), "tILE_skiN") << "Name should not actually be the same as the one in the request";
    EXPECT_EQ(tileSkin->getDeclFilePath(), "skins/test_skins.skin");
    EXPECT_EQ(tileSkin->getRemap("textures/atest/a"), "textures/numbers/10");
}

inline bool containsRemap(const std::vector<decl::ISkin::Remapping>& remaps, 
    const std::string& original, const std::string& replacement)
{
    for (const auto& remap : remaps)
    {
        if (remap.Original == original && remap.Replacement == replacement)
        {
            return true;
        }
    }

    return false;
}

TEST_F(ModelSkinTest, GetAllRemaps)
{
    auto skin = GlobalModelSkinCache().findSkin("skin_with_wildcard");

    const auto& remaps = skin->getAllRemappings();

    EXPECT_EQ(remaps.size(), 2);
    EXPECT_TRUE(containsRemap(remaps, "textures/common/caulk", "textures/common/shadowcaulk"));
    EXPECT_TRUE(containsRemap(remaps, "*", "textures/common/nodraw"));
}

TEST_F(ModelSkinTest, GetModels)
{
    // Skin without any models listed
    auto skin = GlobalModelSkinCache().findSkin("skin_with_wildcard");
    EXPECT_EQ(skin->getModels().size(), 0);

    // Skin with 2 models
    skin = GlobalModelSkinCache().findSkin("separated_tile_skin");
    const auto& models = skin->getModels();
    EXPECT_EQ(models.size(), 2);
    EXPECT_EQ(models.count("models/ase/separated_tiles.ase"), 1);
    EXPECT_EQ(models.count("models/ase/separated_tiles22.ase"), 1);
}

TEST_F(ModelSkinTest, GetRemap)
{
    auto tileSkin = GlobalModelSkinCache().findSkin("tile_skin2");

    EXPECT_EQ(tileSkin->getRemap("textures/atest/a"), "textures/numbers/12");
    EXPECT_EQ(tileSkin->getRemap("any_other_texture"), "") << "Missing remap should return an empty string";
}

TEST_F(ModelSkinTest, GetRemapUsingWildcard)
{
    auto skin = GlobalModelSkinCache().findSkin("invisible");

    // Check the skin contains what need in this test
    EXPECT_NE(skin->getBlockSyntax().contents.find("*   textures/common/nodraw"), std::string::npos);

    EXPECT_EQ(skin->getRemap("textures/atest/a"), "textures/common/nodraw") << "Skin should always return nodraw";
    EXPECT_EQ(skin->getRemap("any_other_texture"), "textures/common/nodraw") << "Skin should always return nodraw";
}

TEST_F(ModelSkinTest, GetRemapIsRespectingDeclarationOrder)
{
    auto skin = GlobalModelSkinCache().findSkin("skin_with_wildcard");

    EXPECT_EQ(skin->getRemap("textures/common/caulk"), "textures/common/shadowcaulk") << "Skin should respond to specific material first";
    EXPECT_EQ(skin->getRemap("any_other_texture"), "textures/common/nodraw") << "Skin should return nodraw for the rest";
}

inline void expectSkinIsListed(const StringList& skins, const std::string& expectedSkin)
{
    EXPECT_NE(std::find(skins.begin(), skins.end(), expectedSkin), skins.end())
        << "Couldn't find the expected skin " << expectedSkin << " in the list";
}

TEST_F(ModelSkinTest, FindMatchingSkins)
{
    auto separatedSkins = GlobalModelSkinCache().getSkinsForModel("models/ase/separated_tiles.ase");
    EXPECT_EQ(separatedSkins.size(), 1);
    EXPECT_EQ(separatedSkins.at(0), "separated_tile_skin");

    auto tileSkins = GlobalModelSkinCache().getSkinsForModel("models/ase/tiles.ase");
    EXPECT_EQ(tileSkins.size(), 4);
    expectSkinIsListed(tileSkins, "tile_skin");
    expectSkinIsListed(tileSkins, "tile_skin2");
    expectSkinIsListed(tileSkins, "skin_declared_in_pk4");
    expectSkinIsListed(tileSkins, "another_skin_declared_in_pk4");
}

TEST_F(ModelSkinTest, GetAllSkins)
{
    auto allSkins = GlobalModelSkinCache().getAllSkins();
    expectSkinIsListed(allSkins, "tile_skin");
    expectSkinIsListed(allSkins, "tile_skin2");
    expectSkinIsListed(allSkins, "separated_tile_skin");
    expectSkinIsListed(allSkins, "skin_with_strange_casing");
    expectSkinIsListed(allSkins, "ivy_onesided");
}

TEST_F(ModelSkinTest, AddModel)
{
    // Skin with 2 models
    auto skin = GlobalModelSkinCache().findSkin("separated_tile_skin");
        
    EXPECT_EQ(skin->getModels().size(), 2) << "Unit test skin setup wrong";
    EXPECT_FALSE(skin->isModified()) << "Skin should be unmodified at start";
    auto previousDeclBlock = skin->getBlockSyntax().contents;

    std::size_t signalCount = 0;
    skin->signal_DeclarationChanged().connect([&] { ++signalCount; });

    constexpr auto modelToAdd = "models/something.ase";
    EXPECT_EQ(previousDeclBlock.find(modelToAdd), std::string::npos) << "Source should not contain " << modelToAdd;

    skin->addModel(modelToAdd);

    EXPECT_EQ(skin->getModels().size(), 3) << "Model has not been added";
    EXPECT_EQ(skin->getModels().count(modelToAdd), 1) << "Model has not been added";
    EXPECT_TRUE(skin->isModified()) << "Skin should be modified now";
    EXPECT_EQ(signalCount, 1) << "Signal has not been emitted";
    EXPECT_NE(skin->getBlockSyntax().contents.find(modelToAdd), std::string::npos) << "Source should contain " << modelToAdd;
}

TEST_F(ModelSkinTest, AddRedundantModel)
{
    // Skin with 2 models
    auto skin = GlobalModelSkinCache().findSkin("separated_tile_skin");
    EXPECT_FALSE(skin->isModified()) << "Skin should be unmodified at start";

    auto previousModelCount = skin->getModels().size();
    auto existingModel = *skin->getModels().begin();

    std::size_t signalCount = 0;
    skin->signal_DeclarationChanged().connect([&] { ++signalCount; });

    skin->addModel(existingModel);

    EXPECT_EQ(skin->getModels().size(), previousModelCount) << "Model count should not have been changed";
    EXPECT_FALSE(skin->isModified()) << "Skin should still be unmodified";
    EXPECT_EQ(signalCount, 0) << "Signal should not have been emitted";
}

TEST_F(ModelSkinTest, RemoveModel)
{
    // Skin with 2 models
    auto skin = GlobalModelSkinCache().findSkin("separated_tile_skin");
    EXPECT_FALSE(skin->isModified()) << "Skin should be unmodified at start";

    auto previousModelCount = skin->getModels().size();
    auto existingModel = *skin->getModels().begin();

    std::size_t signalCount = 0;
    skin->signal_DeclarationChanged().connect([&] { ++signalCount; });

    skin->removeModel(existingModel);

    EXPECT_EQ(skin->getModels().size(), previousModelCount - 1) << "Model count should have been changed";
    EXPECT_EQ(skin->getModels().count(existingModel), 0) << "Model should be gone now";
    EXPECT_TRUE(skin->isModified()) << "Skin should be modified now";
    EXPECT_EQ(signalCount, 1) << "Signal should have been emitted";
}

TEST_F(ModelSkinTest, ClearRemappings)
{
    // Skin with 2 models
    auto skin = GlobalModelSkinCache().findSkin("separated_tile_skin");
    EXPECT_FALSE(skin->isModified()) << "Skin should be unmodified at start";
    EXPECT_FALSE(skin->getAllRemappings().empty()) << "Remappings should not be empty at start";

    std::size_t signalCount = 0;
    skin->signal_DeclarationChanged().connect([&] { ++signalCount; });

    skin->clearRemappings();

    EXPECT_TRUE(skin->getAllRemappings().empty()) << "Remappings should be empty now";
    EXPECT_TRUE(skin->isModified()) << "Skin should be modified now";
    EXPECT_EQ(signalCount, 1) << "Signal should have been emitted";

    // Clearing an empty remapping list doesn't do anything
    skin->clearRemappings();
    EXPECT_EQ(signalCount, 1) << "Signal should not have been emitted again";
}

TEST_F(ModelSkinTest, AddRemapping)
{
    // Skin with 2 models
    auto skin = GlobalModelSkinCache().findSkin("separated_tile_skin");
    EXPECT_FALSE(skin->isModified()) << "Skin should be unmodified at start";

    constexpr auto original = "texture/original";
    constexpr auto replacement = "texture/replacement";
    EXPECT_FALSE(containsRemap(skin->getAllRemappings(), original, replacement));

    std::size_t signalCount = 0;
    skin->signal_DeclarationChanged().connect([&] { ++signalCount; });

    skin->addRemapping(decl::ISkin::Remapping{ original, replacement });

    EXPECT_TRUE(containsRemap(skin->getAllRemappings(), original, replacement)) << "Remapping has not been added";
    EXPECT_TRUE(skin->isModified()) << "Skin should be modified now";
    EXPECT_EQ(signalCount, 1) << "Signal should have been emitted";

    // Adding the same again doesn't change anything
    skin->addRemapping(decl::ISkin::Remapping{ original, replacement });
    EXPECT_EQ(signalCount, 1) << "Signal should not have been emitted";
}

TEST_F(ModelSkinTest, RemoveRemapping)
{
    // Skin with 2 models
    auto skin = GlobalModelSkinCache().findSkin("separated_tile_skin");
    EXPECT_FALSE(skin->isModified()) << "Skin should be unmodified at start";

    auto original = skin->getAllRemappings().begin()->Original;
    auto replacement = skin->getAllRemappings().begin()->Replacement;
    EXPECT_TRUE(containsRemap(skin->getAllRemappings(), original, replacement)) << "Remapping not found?";

    std::size_t signalCount = 0;
    skin->signal_DeclarationChanged().connect([&] { ++signalCount; });

    skin->removeRemapping(original);

    EXPECT_FALSE(containsRemap(skin->getAllRemappings(), original, replacement)) << "Remapping has not been removed";
    EXPECT_TRUE(skin->isModified()) << "Skin should be modified now";
    EXPECT_EQ(signalCount, 1) << "Signal should have been emitted";

    // Removing again doesn't change anything
    skin->removeRemapping(original);
    EXPECT_EQ(signalCount, 1) << "Signal should not have been emitted";

    // Removing a non-existent mapping doesn't do anything either
    skin->removeRemapping("nonexistent");
    EXPECT_EQ(signalCount, 1) << "Signal should not have been emitted";
}

TEST_F(ModelSkinTest, RemoveNonexistentModel)
{
    // Skin with 2 models
    auto skin = GlobalModelSkinCache().findSkin("separated_tile_skin");
    EXPECT_FALSE(skin->isModified()) << "Skin should be unmodified at start";

    auto previousModelCount = skin->getModels().size();
    constexpr auto nonexistentModel = "nonexistent_model.ase";

    std::size_t signalCount = 0;
    skin->signal_DeclarationChanged().connect([&] { ++signalCount; });

    skin->removeModel(nonexistentModel);

    EXPECT_EQ(skin->getModels().size(), previousModelCount) << "Model count should not have been changed";
    EXPECT_FALSE(skin->isModified()) << "Skin should still be unmodified";
    EXPECT_EQ(signalCount, 0) << "Signal should not have been emitted";
}

TEST_F(ModelSkinTest, RevertChanges)
{
    // Skin with 2 models
    auto skin = GlobalModelSkinCache().findSkin("separated_tile_skin");
    EXPECT_FALSE(skin->isModified()) << "Skin should be unmodified at start";

    auto previousModels = skin->getModels();

    std::size_t signalCount = 0;
    skin->signal_DeclarationChanged().connect([&] { ++signalCount; });

    constexpr auto modelToAdd = "models/something.ase";
    skin->addModel(modelToAdd);

    EXPECT_TRUE(skin->isModified()) << "Skin should be modified now";
    EXPECT_NE(skin->getModels(), previousModels) << "Models should have been changed";
    EXPECT_EQ(signalCount, 1) << "Signal has not been emitted";

    skin->revertModifications();

    EXPECT_EQ(skin->getModels(), previousModels) << "Models should be reverted";
    EXPECT_EQ(signalCount, 2) << "Signal should have been emitted";
    EXPECT_FALSE(skin->isModified()) << "Skin should be unmodified again";
}

TEST_F(ModelSkinTest, CommitChanges)
{
    // Skin with 2 models
    auto skin = GlobalModelSkinCache().findSkin("separated_tile_skin");
    EXPECT_FALSE(skin->isModified()) << "Skin should be unmodified at start";

    auto previousModels = skin->getModels();

    std::size_t signalCount = 0;
    skin->signal_DeclarationChanged().connect([&] { ++signalCount; });

    constexpr auto modelToAdd = "models/something.ase";
    skin->addModel(modelToAdd);

    EXPECT_TRUE(skin->isModified()) << "Skin should be modified now";
    EXPECT_NE(skin->getModels(), previousModels) << "Models should have been changed";
    EXPECT_EQ(signalCount, 1) << "Signal has not been emitted";

    skin->commitModifications();

    previousModels.insert(modelToAdd);
    EXPECT_EQ(skin->getModels(), previousModels) << "Model list should contain the new model";
    EXPECT_EQ(signalCount, 2) << "Signal should have been emitted";
    EXPECT_FALSE(skin->isModified()) << "Skin should be unmodified again";
}

TEST_F(ModelSkinTest, RenameSkin)
{
    constexpr auto oldName = "separated_tile_skin";
    constexpr auto newName = "newSkinName";
    auto skin = GlobalModelSkinCache().findSkin(oldName);
    EXPECT_FALSE(skin->isModified()) << "Skin should be unmodified at start";

    std::size_t signalCount = 0;
    skin->signal_DeclarationChanged().connect([&] { ++signalCount; });

    GlobalModelSkinCache().renameSkin(oldName, newName);

    EXPECT_TRUE(skin->isModified()) << "Skin should be modified now";
    EXPECT_EQ(skin->getDeclName(), newName) << "Skin name should have been changed";
    EXPECT_EQ(skin->getOriginalDeclName(), oldName) << "Original name should stay the same";
    EXPECT_EQ(signalCount, 1) << "Signal has not been emitted";

    EXPECT_TRUE(GlobalModelSkinCache().findSkin(newName)) << "Lookup by the new name should have succeeded";
    EXPECT_FALSE(GlobalModelSkinCache().findSkin(oldName)) << "Lookup by the old name should not have succeeded";
}

TEST_F(ModelSkinTest, RevertSkinRename)
{
    constexpr auto oldName = "separated_tile_skin";
    constexpr auto newName = "newSkinName";
    auto skin = GlobalModelSkinCache().findSkin(oldName);
    EXPECT_FALSE(skin->isModified()) << "Skin should be unmodified at start";

    GlobalModelSkinCache().renameSkin(oldName, newName);

    EXPECT_TRUE(skin->isModified()) << "Skin should be modified now";

    std::size_t signalCount = 0;
    skin->signal_DeclarationChanged().connect([&] { ++signalCount; });

    skin->revertModifications();

    EXPECT_FALSE(skin->isModified()) << "Skin should be unmodified again";
    EXPECT_EQ(signalCount, 1) << "Signal has not been emitted";

    EXPECT_TRUE(GlobalModelSkinCache().findSkin(oldName)) << "Lookup by the old name should have succeeded";
    EXPECT_FALSE(GlobalModelSkinCache().findSkin(newName)) << "Lookup by the new name should not have succeeded";
}

TEST_F(ModelSkinTest, SkinCanBeModified)
{
    EXPECT_FALSE(GlobalModelSkinCache().skinCanBeModified("skin_declared_in_pk4")) << "PK4 skin is not writable";
    EXPECT_FALSE(GlobalModelSkinCache().skinCanBeModified("another_skin_declared_in_pk4")) << "PK4 skin is not writable";
    EXPECT_TRUE(GlobalModelSkinCache().skinCanBeModified("tile_skin")) << "Filesystem skin is writable";

    EXPECT_FALSE(GlobalModelSkinCache().skinCanBeModified("nonexistent_skin")) << "Nonexistent skin is not writable";
}

TEST_F(ModelSkinTest, CopySkin)
{
    auto& skinManager = GlobalModelSkinCache();

    std::string receivedName;
    decl::Type receivedType;

    GlobalDeclarationManager().signal_DeclCreated().connect([&](decl::Type type, const std::string& name)
    {
        receivedName = name;
        receivedType = type;
    });

    auto originalSkin = skinManager.findSkin("separated_tile_skin");
    EXPECT_TRUE(originalSkin);
    EXPECT_FALSE(skinManager.findSkin("skin/copytest"));

    // Copy name must not be empty => returns empty material
    EXPECT_FALSE(skinManager.copySkin("separated_tile_skin", ""));

    // Source material name must be existent
    EXPECT_FALSE(skinManager.copySkin("skin/menotexist", "skin/copytest"));

    // This copy operation should succeed
    auto skin = skinManager.copySkin("separated_tile_skin", "skin/copytest");
    EXPECT_TRUE(skin) << "No skin copy has been created";
    EXPECT_EQ(skin->getDeclName(), "skin/copytest") << "Wrong name assigned";
    EXPECT_EQ(skin->getBlockSyntax().contents, originalSkin->getBlockSyntax().contents) << "Contents not copied";
    EXPECT_TRUE(skin->isModified()) << "Copied skin should be set to modified";
    EXPECT_TRUE(skinManager.skinCanBeModified(skin->getDeclName()));
    EXPECT_EQ(skin->getBlockSyntax().fileInfo.name, "");
    EXPECT_EQ(skin->getBlockSyntax().fileInfo.topDir, "");
    EXPECT_EQ(skin->getBlockSyntax().getModName(), GlobalGameManager().currentGame()->getName())
        << "Copy should have the current game name set, since we don't have a mod in the unit tests";

    // Check signal emission
    EXPECT_EQ(receivedName, skin->getDeclName()) << "Wrong name in signal";
    EXPECT_EQ(receivedType, skin->getDeclType()) << "Wrong type in signal";

    // Creating another copy will create a new name
    auto skin2 = skinManager.copySkin("separated_tile_skin", "skin/copytest");
    EXPECT_TRUE(skin2);
    EXPECT_NE(skin2->getDeclName(), skin->getDeclName()) << "Conflicting name assigned";
}

TEST_F(ModelSkinTest, ReloadDeclsRefreshesModels)
{
    // Create a temporary file holding a new skin
    TemporaryFile tempFile(_context.getTestProjectPath() + "skins/_skin_decl_test.skin");
    tempFile.setContents(R"(
skin temporary_skin
{
    model               models/ase/tiles.ase
    textures/atest/a    textures/common/caulk
}
)");

    // Load that new declaration
    GlobalDeclarationManager().reloadDeclarations();

    EXPECT_TRUE(GlobalModelSkinCache().findSkin("temporary_skin"));

    // Create a model and insert it into the scene
    auto funcStaticClass = GlobalEntityClassManager().findClass("func_static");
    auto funcStatic = GlobalEntityModule().createEntity(funcStaticClass);
    scene::addNodeToContainer(funcStatic, GlobalMapModule().getRoot());

    // Set model and skin spawnargs
    funcStatic->getEntity().setKeyValue("model", "models/ase/tiles.ase");
    funcStatic->getEntity().setKeyValue("skin", "temporary_skin");

    // Find the child model node
    auto model = algorithm::findChildModel(funcStatic);
    EXPECT_EQ(model->getIModel().getModelPath(), "models/ase/tiles.ase");

    // Check the contents of the materials list
    auto activeMaterials = model->getIModel().getActiveMaterials();
    EXPECT_EQ(activeMaterials.size(), 1);
    EXPECT_EQ(activeMaterials.at(0), "textures/common/caulk");

    // Change the remap to noclip
    tempFile.setContents(R"(
skin temporary_skin
{
    model               models/ase/tiles.ase
    textures/atest/a    textures/common/noclip
}
)");

    GlobalDeclarationManager().reloadDeclarations();

    activeMaterials = model->getIModel().getActiveMaterials();
    EXPECT_EQ(activeMaterials.size(), 1);
    EXPECT_EQ(activeMaterials.at(0), "textures/common/noclip");
}

// A slight variant of the above ReloadDeclsRefreshesModels: in this test the
// skin body is changed manually (similar to what happens during reloadDecls)
// and the declsReloaded signal is fired manually by the test code.
// This is to hide the dominant entityDefs-reloaded signal that is making
// the above test green without even without any skin signal listening code
TEST_F(ModelSkinTest, ReloadDeclsRefreshesModelsUsingSignal)
{
    // Create a model and insert it into the scene
    auto funcStaticClass = GlobalEntityClassManager().findClass("func_static");
    auto funcStatic = GlobalEntityModule().createEntity(funcStaticClass);
    scene::addNodeToContainer(funcStatic, GlobalMapModule().getRoot());

    // Set model and skin spawnargs
    funcStatic->getEntity().setKeyValue("model", "models/ase/tiles.ase");
    funcStatic->getEntity().setKeyValue("skin", "tile_skin");

    // Find the child model node
    auto model = algorithm::findChildModel(funcStatic);
    EXPECT_EQ(model->getIModel().getModelPath(), "models/ase/tiles.ase");

    // Check the contents of the materials list
    auto activeMaterials = model->getIModel().getActiveMaterials();
    EXPECT_EQ(activeMaterials.size(), 1);
    EXPECT_EQ(activeMaterials.at(0), "textures/numbers/10");

    // Inject a new block syntax into the existing skin declaration
    auto skin = GlobalModelSkinCache().findSkin("tile_skin");
    decl::DeclarationBlockSyntax syntax;

    syntax.name = skin->getDeclName();
    syntax.typeName = "skin";
    syntax.contents = R"(
    model               models/ase/tiles.ase
    textures/atest/a    textures/common/noclip
)";
    skin->setBlockSyntax(syntax);

    // Create an artificial skins-reloaded event
    GlobalDeclarationManager().signal_DeclsReloaded(decl::Type::Skin).emit();

    activeMaterials = model->getIModel().getActiveMaterials();
    EXPECT_EQ(activeMaterials.size(), 1);
    EXPECT_EQ(activeMaterials.at(0), "textures/common/noclip");
}

TEST_F(ModelSkinTest, SkinIsUnlistedAfterSkinRemoval)
{
    constexpr auto skinToRemove = "tile_skin2";
    constexpr auto associatedModel = "models/ase/tiles.ase";

    auto decl = GlobalDeclarationManager().findDeclaration(decl::Type::Skin, skinToRemove);
    EXPECT_TRUE(decl);

    // Check prerequisites, skin should be listed
    auto allSkins = GlobalModelSkinCache().getAllSkins();
    EXPECT_NE(std::find(allSkins.begin(), allSkins.end(), skinToRemove), allSkins.end());

    // Skin should be reported as matching skin for a model
    auto skinsForModel = GlobalModelSkinCache().getSkinsForModel(associatedModel);
    EXPECT_NE(std::find(skinsForModel.begin(), skinsForModel.end(), skinToRemove), skinsForModel.end());

    // Create a backup copy of the decl file we're going to manipulate
    BackupCopy backup(_context.getTestProjectPath() + decl->getDeclFilePath());

    GlobalDeclarationManager().removeDeclaration(decl->getDeclType(), decl->getDeclName());

    // The skin should disappear from both skin lists
    allSkins = GlobalModelSkinCache().getAllSkins();
    EXPECT_EQ(std::find(allSkins.begin(), allSkins.end(), skinToRemove), allSkins.end())
        << "Skin should have disappeared from the all skins list";

    skinsForModel = GlobalModelSkinCache().getSkinsForModel(associatedModel);
    EXPECT_EQ(std::find(skinsForModel.begin(), skinsForModel.end(), skinToRemove), skinsForModel.end())
        << "Skin should have disappeared from the model list";
}

TEST_F(ModelSkinTest, SkinIsListedAfterSkinCreation)
{
    constexpr auto skinToAdd = "some/new/skin";

    EXPECT_FALSE(GlobalDeclarationManager().findDeclaration(decl::Type::Skin, skinToAdd));

    // Check prerequisites, skin should not be listed
    auto allSkins = GlobalModelSkinCache().getAllSkins();
    EXPECT_EQ(std::find(allSkins.begin(), allSkins.end(), skinToAdd), allSkins.end());

    GlobalDeclarationManager().findOrCreateDeclaration(decl::Type::Skin, skinToAdd);

    // The skin should appear on the all skins lists
    allSkins = GlobalModelSkinCache().getAllSkins();
    EXPECT_NE(std::find(allSkins.begin(), allSkins.end(), skinToAdd), allSkins.end())
        << "Skin should appear on the all skins list";
}

TEST_F(ModelSkinTest, SkinIsListedAfterSkinCopy)
{
    constexpr auto skinToCopy = "tile_skin2";

    auto originalSkin = GlobalModelSkinCache().findSkin(skinToCopy);
    EXPECT_TRUE(originalSkin);
    auto associatedModel = *originalSkin->getModels().begin();

    auto nameOfCopy = "some/copied/skin";

    // Check prerequisites, skin should not be listed
    auto allSkins = GlobalModelSkinCache().getAllSkins();
    EXPECT_EQ(std::find(allSkins.begin(), allSkins.end(), nameOfCopy), allSkins.end());

    // Skin should not be reported as matching skin for a model
    auto skinsForModel = GlobalModelSkinCache().getSkinsForModel(associatedModel);
    EXPECT_EQ(std::find(skinsForModel.begin(), skinsForModel.end(), nameOfCopy), skinsForModel.end());

    GlobalModelSkinCache().copySkin(skinToCopy, nameOfCopy);

    // The skin should appear on both skin lists now
    allSkins = GlobalModelSkinCache().getAllSkins();
    EXPECT_NE(std::find(allSkins.begin(), allSkins.end(), nameOfCopy), allSkins.end())
        << "Skin should appear on the all skins list";

    skinsForModel = GlobalModelSkinCache().getSkinsForModel(associatedModel);
    EXPECT_NE(std::find(skinsForModel.begin(), skinsForModel.end(), nameOfCopy), skinsForModel.end())
        << "Skin should appear on the model list";
}

// Modifying a skin might affect the list of models it is associated with
TEST_F(ModelSkinTest, SkinIsListedAfterSkinChange)
{
    constexpr auto skinToModify = "tile_skin2";

    auto originalSkin = GlobalModelSkinCache().findSkin(skinToModify);
    EXPECT_TRUE(originalSkin);
    auto associatedModel = *originalSkin->getModels().begin();

    // Skin should be reported as matching skin for a model
    auto skinsForModel = GlobalModelSkinCache().getSkinsForModel(associatedModel);
    EXPECT_NE(std::find(skinsForModel.begin(), skinsForModel.end(), skinToModify), skinsForModel.end());

    // Modify the skin, check the result
    originalSkin->removeModel(associatedModel);

    skinsForModel = GlobalModelSkinCache().getSkinsForModel(associatedModel);
    EXPECT_EQ(std::find(skinsForModel.begin(), skinsForModel.end(), skinToModify), skinsForModel.end())
        << "Skin should have disappeared from the matching skin list";

    // Modify the skin again, adding it back
    auto syntax = originalSkin->getBlockSyntax();
    syntax.contents = "\n\nmodel \"" + associatedModel + "\"\n\n" + syntax.contents;
    originalSkin->setBlockSyntax(syntax);

    skinsForModel = GlobalModelSkinCache().getSkinsForModel(associatedModel);
    EXPECT_NE(std::find(skinsForModel.begin(), skinsForModel.end(), skinToModify), skinsForModel.end())
        << "Skin should have reappeared on the matching skin list";
}

// Renaming a skin affects the internal lists in the global model skin cache
TEST_F(ModelSkinTest, SkinIsListedAfterSkinRename)
{
    constexpr auto skinToRename = "tile_skin2";
    constexpr auto newName = "some/new/skin";

    auto originalSkin = GlobalModelSkinCache().findSkin(skinToRename);
    EXPECT_TRUE(originalSkin);
    auto associatedModel = *originalSkin->getModels().begin();

    // Check prerequisites, skin should be listed
    auto allSkins = GlobalModelSkinCache().getAllSkins();
    EXPECT_NE(std::find(allSkins.begin(), allSkins.end(), skinToRename), allSkins.end());

    // Skin should be reported as matching skin for a model
    auto skinsForModel = GlobalModelSkinCache().getSkinsForModel(associatedModel);
    EXPECT_NE(std::find(skinsForModel.begin(), skinsForModel.end(), skinToRename), skinsForModel.end());

    GlobalModelSkinCache().renameSkin(skinToRename, newName);

    // The skin should appear with the new name now
    allSkins = GlobalModelSkinCache().getAllSkins();
    EXPECT_NE(std::find(allSkins.begin(), allSkins.end(), newName), allSkins.end())
        << "Skin should be listed with its new name";
    EXPECT_EQ(std::find(allSkins.begin(), allSkins.end(), skinToRename), allSkins.end())
        << "Old skin name should be gone now";

    skinsForModel = GlobalModelSkinCache().getSkinsForModel(associatedModel);
    EXPECT_NE(std::find(skinsForModel.begin(), skinsForModel.end(), newName), skinsForModel.end())
        << "New skin name should be associated now";
    EXPECT_EQ(std::find(skinsForModel.begin(), skinsForModel.end(), skinToRename), skinsForModel.end())
        << "Old skin name should not be associated";
}

void expectModelDefHasMeshAndSkin(const std::string& modelDef, const std::string& expectedMesh, const std::string& expectedSkin)
{
    auto model = GlobalEntityClassManager().findModel(modelDef);
    EXPECT_EQ(model->getMesh(), expectedMesh) << "Expected mesh to be " << expectedMesh << " on modelDef " << modelDef;
    EXPECT_EQ(model->getSkin(), expectedSkin) << "Expected skin to be " << expectedSkin << " on modelDef " << modelDef;
}

IEntityNodePtr createStaticEntityWithModel(const std::string& model)
{
    auto funcStaticClass = GlobalEntityClassManager().findClass("func_static");
    auto entity = GlobalEntityModule().createEntity(funcStaticClass);
    entity->getEntity().setKeyValue("model", model);
    return entity;
}

SkinnedModel::Ptr getSkinnedModel(const IEntityNodePtr& entity)
{
    SkinnedModel::Ptr foundModelNode;

    entity->foreachNode([&](const auto& child)
    {
        auto model = std::dynamic_pointer_cast<SkinnedModel>(child);
        if (!foundModelNode && model)
        {
            foundModelNode = model;
        }
        return true;
    });

    EXPECT_TRUE(foundModelNode) << "Failed to find the skinned model node";

    return foundModelNode;
}

void expectEntityHasSkinnedModel(const IEntityNodePtr& entity, const std::string& expectedSkin, const std::vector<std::string>& expectedMaterials)
{
    // Check the skinned model node beneath it
    auto foundModelNode = getSkinnedModel(entity);
    EXPECT_EQ(foundModelNode->getSkin(), expectedSkin) << "Expected skin to be " << expectedSkin << " on entity with model " << entity->getEntity().getKeyValue("model");

    auto modelNode = std::dynamic_pointer_cast<model::ModelNode>(foundModelNode);

    EXPECT_TRUE(modelNode) << "Cast to ModelNode failed";
    EXPECT_EQ(modelNode->getIModel().getActiveMaterials().size(), expectedMaterials.size()) << "Mismatching number of materials";
    EXPECT_TRUE(modelNode->getIModel().getActiveMaterials() == expectedMaterials) << "Material list mismatch";
}

void expectEntityHasSkinnedModel(const std::string& modelKeyValue, const std::string& expectedSkin, const std::vector<std::string>& expectedMaterials)
{
    auto entity = createStaticEntityWithModel(modelKeyValue);
    expectEntityHasSkinnedModel(entity, expectedSkin, expectedMaterials);
}

void expectEntityClassHasSkinnedModel(const std::string& eclassName, const std::string& expectedSkin, const std::vector<std::string>& expectedMaterials)
{
    auto eclass = GlobalEntityClassManager().findClass(eclassName);
    EXPECT_TRUE(eclass) << "Unable to locate entityDef " << eclassName;
    auto entity = GlobalEntityModule().createEntity(eclass);
    expectEntityHasSkinnedModel(entity, expectedSkin, expectedMaterials);
}

void setSkinKeyAndCheckModel(const IEntityNodePtr& entity, const std::string& expectedSkin, const std::vector<std::string>& expectedMaterials)
{
    // Save the data to do a before/after test
    auto model = getSkinnedModel(entity);
    auto modelNode = std::dynamic_pointer_cast<model::ModelNode>(model);

    auto skinBeforeSettingKey = model->getSkin();
    auto materialsBeforeSettingKey = modelNode->getIModel().getActiveMaterials();

    entity->getEntity().setKeyValue("skin", expectedSkin);
    expectEntityHasSkinnedModel(entity, expectedSkin, expectedMaterials);

    // Remove the skin key again, it should be as before
    entity->getEntity().setKeyValue("skin", "");
    expectEntityHasSkinnedModel(entity, skinBeforeSettingKey, materialsBeforeSettingKey);
}

// An entity using a certain modelDef as "model" and explicitly setting the "skin" spawnarg
void expectEntityWithSkinKeyHasSkinnedModel(const std::string& modelKeyValue, const std::string& expectedSkin, const std::vector<std::string>& expectedMaterials)
{
    auto entity = createStaticEntityWithModel(modelKeyValue);
    setSkinKeyAndCheckModel(entity, expectedSkin, expectedMaterials);
}

void expectEntityClassWithSkinKeyHasSkinnedModel(const std::string& eclassName, const std::string& expectedSkin, const std::vector<std::string>& expectedMaterials)
{
    auto eclass = GlobalEntityClassManager().findClass(eclassName);
    EXPECT_TRUE(eclass) << "Unable to locate entityDef " << eclassName;

    auto entity = GlobalEntityModule().createEntity(eclass);
    setSkinKeyAndCheckModel(entity, expectedSkin, expectedMaterials);
}

namespace
{
    const std::vector<std::string> flagPirateSet = { "flag_pirate", "flag_pirate" };
    const std::string caulkSkin = "swap_flag_pirate_with_caulk";
    const std::vector<std::string> caulkSet = { "textures/common/caulk", "textures/common/caulk" };
    const std::string nodrawSkin = "swap_flag_pirate_with_nodraw";
    const std::vector<std::string> nodrawSet = { "textures/common/nodraw", "textures/common/nodraw" };
    const std::string visportalSkin = "swap_flag_pirate_with_visportal";
    const std::vector<std::string> visportalSet = { "textures/editor/visportal", "textures/editor/visportal" };
    const std::string aasSolidSkin = "swap_flag_pirate_with_aassolid";
    const std::vector<std::string> aasSolidSet = { "textures/editor/aassolid", "textures/editor/aassolid" };
}

// Variations of modelDefs, all of them have a mesh, some of them define a skin, some inherit a skin, some override a skin
TEST_F(ModelSkinTest, ModelDefSkinKeword)
{
    expectModelDefHasMeshAndSkin("some_base_modeldef_without_skin", "models/md5/testflag.md5mesh", "");
    expectModelDefHasMeshAndSkin("some_base_modeldef_with_skin", "models/md5/testflag.md5mesh", caulkSkin);
    expectModelDefHasMeshAndSkin("some_modeldef_inheriting_model_only", "models/md5/testflag.md5mesh", "");
    expectModelDefHasMeshAndSkin("some_modeldef_inheriting_model_and_skin", "models/md5/testflag.md5mesh", caulkSkin);
    expectModelDefHasMeshAndSkin("some_modeldef_overriding_inherited_skin", "models/md5/testflag.md5mesh", nodrawSkin);
}

// Test the resulting skinned model attached to an entity node using the given modelDef as "model"
TEST_F(ModelSkinTest, EntityUsingModelDef)
{
    expectEntityHasSkinnedModel("some_base_modeldef_without_skin", "", flagPirateSet);
    expectEntityHasSkinnedModel("some_base_modeldef_with_skin", caulkSkin, caulkSet);
    expectEntityHasSkinnedModel("some_modeldef_inheriting_model_only", "", flagPirateSet);
    expectEntityHasSkinnedModel("some_modeldef_inheriting_model_and_skin", caulkSkin, caulkSet);
    expectEntityHasSkinnedModel("some_modeldef_overriding_inherited_skin", nodrawSkin, nodrawSet);

    // Try again with the "skin" keyword set as entity key value, overriding all inherited skin settings
    expectEntityWithSkinKeyHasSkinnedModel("some_base_modeldef_without_skin", nodrawSkin, nodrawSet);
    expectEntityWithSkinKeyHasSkinnedModel("some_base_modeldef_with_skin", nodrawSkin, nodrawSet);
    expectEntityWithSkinKeyHasSkinnedModel("some_modeldef_inheriting_model_only", nodrawSkin, nodrawSet);
    expectEntityWithSkinKeyHasSkinnedModel("some_modeldef_inheriting_model_and_skin", nodrawSkin, nodrawSet);
    expectEntityWithSkinKeyHasSkinnedModel("some_modeldef_overriding_inherited_skin", nodrawSkin, nodrawSet);
}

// Test the resulting skinned model attached to an entity node using a certain entityDef (which in turn is using a modelDef as "model" keyword)
TEST_F(ModelSkinTest, EntityClassUsingModelDef)
{
    // Go through all the crazy combinations of entityDefs defined in the skinned_models.def file

    expectEntityClassHasSkinnedModel("entity_using_some_base_modeldef_without_skin", "", flagPirateSet);
    expectEntityClassHasSkinnedModel("entity_using_some_base_modeldef_with_skin", caulkSkin, caulkSet);
    expectEntityClassHasSkinnedModel("entity_using_some_modeldef_inheriting_model_only", "", flagPirateSet);
    expectEntityClassHasSkinnedModel("entity_using_some_modeldef_inheriting_model_and_skin", caulkSkin, caulkSet);
    expectEntityClassHasSkinnedModel("entity_using_some_modeldef_overriding_inherited_skin", nodrawSkin, nodrawSet);

    expectEntityClassHasSkinnedModel("entity_inheriting_some_base_modeldef_without_skin", "", flagPirateSet);
    expectEntityClassHasSkinnedModel("entity_inheriting_entity_using_some_base_modeldef_with_skin", caulkSkin, caulkSet);
    expectEntityClassHasSkinnedModel("entity_inheriting_entity_using_some_modeldef_inheriting_model_only", "", flagPirateSet);
    expectEntityClassHasSkinnedModel("entity_inheriting_entity_using_some_modeldef_inheriting_model_and_skin", caulkSkin, caulkSet);
    expectEntityClassHasSkinnedModel("entity_inheriting_entity_using_some_modeldef_overriding_inherited_skin", nodrawSkin, nodrawSet);

    expectEntityClassHasSkinnedModel("entity_inheriting_some_base_modeldef_without_skin_overriding_skin", nodrawSkin, nodrawSet);
    expectEntityClassHasSkinnedModel("entity_inheriting_entity_using_some_base_modeldef_with_skin_overriding_skin", nodrawSkin, nodrawSet);
    expectEntityClassHasSkinnedModel("entity_inheriting_entity_using_some_modeldef_inheriting_model_only_overriding_skin", nodrawSkin, nodrawSet);
    expectEntityClassHasSkinnedModel("entity_inheriting_entity_using_some_modeldef_inheriting_model_and_skin_overriding_skin", nodrawSkin, nodrawSet);
    expectEntityClassHasSkinnedModel("entity_inheriting_entity_using_some_modeldef_overriding_inherited_skin_overriding_skin", nodrawSkin, nodrawSet);

    expectEntityClassHasSkinnedModel("entity_using_some_base_modeldef_without_skin_overriding_skin", visportalSkin, visportalSet);
    expectEntityClassHasSkinnedModel("entity_using_some_base_modeldef_with_skin_overriding_skin", visportalSkin, visportalSet);
    expectEntityClassHasSkinnedModel("entity_using_some_modeldef_inheriting_model_only_overriding_skin", visportalSkin, visportalSet);
    expectEntityClassHasSkinnedModel("entity_using_some_modeldef_inheriting_model_and_skin_only_overriding_skin", visportalSkin, visportalSet);
    expectEntityClassHasSkinnedModel("entity_using_some_modeldef_overriding_inherited_skin_overriding_skin", visportalSkin, visportalSet);

    expectEntityClassHasSkinnedModel("entity_inheriting_entity_using_some_base_modeldef_without_skin_overriding_skin_overriding_skin", caulkSkin, caulkSet);
    expectEntityClassHasSkinnedModel("entity_inheriting_entity_using_some_base_modeldef_with_skin_overriding_skin_overriding_skin", caulkSkin, caulkSet);
    expectEntityClassHasSkinnedModel("entity_inheriting_entity_using_some_modeldef_inheriting_model_only_overriding_skin_overriding_skin", caulkSkin, caulkSet);
    expectEntityClassHasSkinnedModel("entity_inheriting_entity_using_some_modeldef_inheriting_model_and_skin_only_overriding_skin_overriding_skin", caulkSkin, caulkSet);
    expectEntityClassHasSkinnedModel("entity_inheriting_entity_using_some_modeldef_overriding_inherited_skin_overriding_skin_overriding_skin", caulkSkin, caulkSet);
}

TEST_F(ModelSkinTest, EntityClassUsingModelDefCustomSkinKey)
{
    // Go through all the entityDefs defined in the skinned_models.def file, but set a custom "skin" keyword on the entity
    expectEntityClassWithSkinKeyHasSkinnedModel("entity_using_some_base_modeldef_without_skin", aasSolidSkin, aasSolidSet);
    expectEntityClassWithSkinKeyHasSkinnedModel("entity_using_some_base_modeldef_with_skin", aasSolidSkin, aasSolidSet);
    expectEntityClassWithSkinKeyHasSkinnedModel("entity_using_some_modeldef_inheriting_model_only", aasSolidSkin, aasSolidSet);
    expectEntityClassWithSkinKeyHasSkinnedModel("entity_using_some_modeldef_inheriting_model_and_skin", aasSolidSkin, aasSolidSet);
    expectEntityClassWithSkinKeyHasSkinnedModel("entity_using_some_modeldef_overriding_inherited_skin", aasSolidSkin, aasSolidSet);

    expectEntityClassWithSkinKeyHasSkinnedModel("entity_inheriting_some_base_modeldef_without_skin", aasSolidSkin, aasSolidSet);
    expectEntityClassWithSkinKeyHasSkinnedModel("entity_inheriting_entity_using_some_base_modeldef_with_skin", aasSolidSkin, aasSolidSet);
    expectEntityClassWithSkinKeyHasSkinnedModel("entity_inheriting_entity_using_some_modeldef_inheriting_model_only", aasSolidSkin, aasSolidSet);
    expectEntityClassWithSkinKeyHasSkinnedModel("entity_inheriting_entity_using_some_modeldef_inheriting_model_and_skin", aasSolidSkin, aasSolidSet);
    expectEntityClassWithSkinKeyHasSkinnedModel("entity_inheriting_entity_using_some_modeldef_overriding_inherited_skin", aasSolidSkin, aasSolidSet);

    expectEntityClassWithSkinKeyHasSkinnedModel("entity_inheriting_some_base_modeldef_without_skin_overriding_skin", aasSolidSkin, aasSolidSet);
    expectEntityClassWithSkinKeyHasSkinnedModel("entity_inheriting_entity_using_some_base_modeldef_with_skin_overriding_skin", aasSolidSkin, aasSolidSet);
    expectEntityClassWithSkinKeyHasSkinnedModel("entity_inheriting_entity_using_some_modeldef_inheriting_model_only_overriding_skin", aasSolidSkin, aasSolidSet);
    expectEntityClassWithSkinKeyHasSkinnedModel("entity_inheriting_entity_using_some_modeldef_inheriting_model_and_skin_overriding_skin", aasSolidSkin, aasSolidSet);
    expectEntityClassWithSkinKeyHasSkinnedModel("entity_inheriting_entity_using_some_modeldef_overriding_inherited_skin_overriding_skin", aasSolidSkin, aasSolidSet);

    expectEntityClassWithSkinKeyHasSkinnedModel("entity_using_some_base_modeldef_without_skin_overriding_skin", aasSolidSkin, aasSolidSet);
    expectEntityClassWithSkinKeyHasSkinnedModel("entity_using_some_base_modeldef_with_skin_overriding_skin", aasSolidSkin, aasSolidSet);
    expectEntityClassWithSkinKeyHasSkinnedModel("entity_using_some_modeldef_inheriting_model_only_overriding_skin", aasSolidSkin, aasSolidSet);
    expectEntityClassWithSkinKeyHasSkinnedModel("entity_using_some_modeldef_inheriting_model_and_skin_only_overriding_skin", aasSolidSkin, aasSolidSet);
    expectEntityClassWithSkinKeyHasSkinnedModel("entity_using_some_modeldef_overriding_inherited_skin_overriding_skin", aasSolidSkin, aasSolidSet);

    expectEntityClassWithSkinKeyHasSkinnedModel("entity_inheriting_entity_using_some_base_modeldef_without_skin_overriding_skin_overriding_skin", aasSolidSkin, aasSolidSet);
    expectEntityClassWithSkinKeyHasSkinnedModel("entity_inheriting_entity_using_some_base_modeldef_with_skin_overriding_skin_overriding_skin", aasSolidSkin, aasSolidSet);
    expectEntityClassWithSkinKeyHasSkinnedModel("entity_inheriting_entity_using_some_modeldef_inheriting_model_only_overriding_skin_overriding_skin", aasSolidSkin, aasSolidSet);
    expectEntityClassWithSkinKeyHasSkinnedModel("entity_inheriting_entity_using_some_modeldef_inheriting_model_and_skin_only_overriding_skin_overriding_skin", aasSolidSkin, aasSolidSet);
    expectEntityClassWithSkinKeyHasSkinnedModel("entity_inheriting_entity_using_some_modeldef_overriding_inherited_skin_overriding_skin_overriding_skin", aasSolidSkin, aasSolidSet);
}

// Changing the model key on an entity that has otherwise no "skin" defined on it
// This is to check that the previous default skin is not carried over to the new model
TEST_F(ModelSkinTest, DefaultSkinAppliedWhenChangingModelDef)
{
    auto eclass = GlobalEntityClassManager().findClass("entity_using_some_base_modeldef_with_skin");
    auto entity = GlobalEntityModule().createEntity(eclass);

    // Check the skin, it should be "swap_flag_pirate_with_caulk" as defined in the modelDef
    auto model = getSkinnedModel(entity);
    EXPECT_EQ(model->getSkin(), "swap_flag_pirate_with_caulk") << "Skin should be 'swap_flag_pirate_with_caulk' as defined in the modelDef";

    // Now change the modelDef, this should switch to a new default skin that should be effective immediately
    entity->getEntity().setKeyValue("model", "some_modeldef_overriding_inherited_skin");

    // Re-acquire the model (the previous node will have swapped out)
    model = getSkinnedModel(entity);
    EXPECT_EQ(model->getSkin(), "swap_flag_pirate_with_nodraw") << "Skin should be 'swap_flag_pirate_with_nodraw' as defined in the new modelDef";
}

// Changing the model key to a different value with a skin spawnarg set on the entity
TEST_F(ModelSkinTest, ExplicitSkinPreservedWhenChangingModelDef)
{
    auto eclass = GlobalEntityClassManager().findClass("entity_using_some_base_modeldef_with_skin");
    auto entity = GlobalEntityModule().createEntity(eclass);

    // Check the skin, it should be "swap_flag_pirate_with_caulk" as defined in the modelDef
    expectEntityHasSkinnedModel(entity, caulkSkin, caulkSet);

    // Set the "skin" key to override the model default
    entity->getEntity().setKeyValue("skin", aasSolidSkin);
    expectEntityHasSkinnedModel(entity, aasSolidSkin, aasSolidSet);

    // Now change the modelDef, this should would have the "swap_flag_pirate_with_nodraw" skin
    // but it's still overridden by the entity
    entity->getEntity().setKeyValue("model", "some_modeldef_overriding_inherited_skin");
    expectEntityHasSkinnedModel(entity, aasSolidSkin, aasSolidSet);

    // Remove the "skin" key value, it should fall back to the model's default
    entity->getEntity().setKeyValue("skin", "");
    expectEntityHasSkinnedModel(entity, nodrawSkin, nodrawSet);
}

// Changing the model key to a different value with a skin spawnarg set on the entityDef
TEST_F(ModelSkinTest, ExplicitInheritedSkinPreservedWhenChangingModelDef)
{
    auto eclass = GlobalEntityClassManager().findClass("entity_using_some_base_modeldef_with_skin_overriding_skin");
    auto entity = GlobalEntityModule().createEntity(eclass);

    // Check the skin, it should be "swap_flag_pirate_with_visportal" as defined in the entityDef
    expectEntityHasSkinnedModel(entity, visportalSkin, visportalSet);

    // Just swap the modelDef without defining any skin (this would have nodraw as default skin)
    entity->getEntity().setKeyValue("model", "some_modeldef_overriding_inherited_skin");
    // This should still use the inherited "skin" property as defined in the entityDef
    expectEntityHasSkinnedModel(entity, visportalSkin, visportalSet);

    // Set the "skin" key to override the model default
    entity->getEntity().setKeyValue("skin", aasSolidSkin);
    expectEntityHasSkinnedModel(entity, aasSolidSkin, aasSolidSet);

    // Remove the "skin" key value, it should fall back to the inherited entityDef
    entity->getEntity().setKeyValue("skin", "");
    expectEntityHasSkinnedModel(entity, visportalSkin, visportalSet);
}

// Change a modelDef's skin and hit reload Decls
TEST_F(ModelSkinTest, SkinChangeDetectionOnReloadDecls)
{
    auto eclass = GlobalEntityClassManager().findClass("entity_using_some_base_modeldef_with_skin");
    auto entity = GlobalEntityModule().createEntity(eclass);

    // Check the skin, it should be "swap_flag_pirate_with_caulk" as defined in the modelDef
    expectEntityHasSkinnedModel(entity, caulkSkin, caulkSet);

    // Hit reload decls globally, this shouldn't have any effect
    GlobalDeclarationManager().reloadDeclarations();
    expectEntityHasSkinnedModel(entity, caulkSkin, caulkSet);

    // Change the def contents, which should trigger a modelDefChanged signal on the entity
    auto modelDef = GlobalDeclarationManager().findDeclaration(decl::Type::ModelDef, "some_base_modeldef_with_skin");
    auto syntax = modelDef->getBlockSyntax();
    string::replace_first(syntax.contents, caulkSkin, visportalSkin);
    modelDef->setBlockSyntax(syntax);

    expectEntityHasSkinnedModel(entity, visportalSkin, visportalSet);
}

// Changing a modelDef's skin should be ignore if the entity has an explicit "skin" property set
TEST_F(ModelSkinTest, SkinChangeInDefOverriddenWithSkinProperty)
{
    auto eclass = GlobalEntityClassManager().findClass("entity_using_some_base_modeldef_with_skin_overriding_skin");
    auto entity = GlobalEntityModule().createEntity(eclass);

    // Check the skin, it should be "swap_flag_pirate_with_visportal" as defined in the entityDef
    expectEntityHasSkinnedModel(entity, visportalSkin, visportalSet);

    // Hit reload decls globally, this shouldn't have any effect
    GlobalDeclarationManager().reloadDeclarations();
    expectEntityHasSkinnedModel(entity, visportalSkin, visportalSet);

    // Change the def contents, which should trigger a modelDefChanged signal on the entity
    // => shouldn't do anything, since the entity is overriding it
    auto modelDef = GlobalDeclarationManager().findDeclaration(decl::Type::ModelDef, "some_base_modeldef_with_skin");
    auto syntax = modelDef->getBlockSyntax();
    string::replace_first(syntax.contents, visportalSkin, aasSolidSkin);
    modelDef->setBlockSyntax(syntax);

    // Still visportal skinned
    expectEntityHasSkinnedModel(entity, visportalSkin, visportalSet);
}

}
