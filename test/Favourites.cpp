#include "RadiantTest.h"

#include "ifavourites.h"
#include "iregistry.h"
#include "settings/SettingsManager.h"

namespace test
{

class FavouritesTest :
    public RadiantTest
{
protected:
    // A function that is invoked after modules have been shut down
    std::function<void()> checkAfterShutdown;

    void copyUserXmlFileToSettingsPath(const std::string& userXmlFile)
    {
        fs::path sourcePath = _context.getTestResourcePath();
        sourcePath /= "settings/";
        sourcePath /= userXmlFile;

        settings::SettingsManager manager(_context);
        fs::path targetPath = manager.getCurrentVersionSettingsFolder();
        targetPath /= "user.xml";

        fs::remove(targetPath);
        fs::copy(sourcePath, targetPath);
    }

    void postShutdown() override
    {
        if (checkAfterShutdown)
        {
            checkAfterShutdown();
        }
    }
};

class FavouritesTestWithLegacyFavourites :
    public FavouritesTest
{
public:
    void preStartup() override
    {
        copyUserXmlFileToSettingsPath("old_mediabrowser_favourites.xml");
    }
};

TEST_F(FavouritesTest, AddingAndRemovingFavourites)
{
    EXPECT_TRUE(GlobalFavouritesManager().getFavourites(decl::Type::Material).empty());

    // Add caulk
    GlobalFavouritesManager().addFavourite(decl::Type::Material, "textures/common/caulk");

    EXPECT_EQ(GlobalFavouritesManager().getFavourites(decl::Type::Material).size(), 1);
    EXPECT_EQ(GlobalFavouritesManager().getFavourites(decl::Type::Material).count("textures/common/caulk"), 1);
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite(decl::Type::Material, "textures/common/caulk"));
    EXPECT_FALSE(GlobalFavouritesManager().isFavourite(decl::Type::Material, "textures/common/clip"));

    // Add clip
    GlobalFavouritesManager().addFavourite(decl::Type::Material, "textures/common/clip");

    EXPECT_EQ(GlobalFavouritesManager().getFavourites(decl::Type::Material).size(), 2);
    EXPECT_EQ(GlobalFavouritesManager().getFavourites(decl::Type::Material).count("textures/common/caulk"), 1);
    EXPECT_EQ(GlobalFavouritesManager().getFavourites(decl::Type::Material).count("textures/common/clip"), 1);
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite(decl::Type::Material, "textures/common/caulk"));
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite(decl::Type::Material, "textures/common/clip"));

    // Remove caulk
    GlobalFavouritesManager().removeFavourite(decl::Type::Material, "textures/common/caulk");

    EXPECT_EQ(GlobalFavouritesManager().getFavourites(decl::Type::Material).size(), 1);
    EXPECT_EQ(GlobalFavouritesManager().getFavourites(decl::Type::Material).count("textures/common/clip"), 1);
    EXPECT_FALSE(GlobalFavouritesManager().isFavourite(decl::Type::Material, "textures/common/caulk"));
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite(decl::Type::Material, "textures/common/clip"));

    // Remove clip
    GlobalFavouritesManager().removeFavourite(decl::Type::Material, "textures/common/clip");

    EXPECT_EQ(GlobalFavouritesManager().getFavourites(decl::Type::Material).size(), 0);
    EXPECT_FALSE(GlobalFavouritesManager().isFavourite(decl::Type::Material, "textures/common/caulk"));
    EXPECT_FALSE(GlobalFavouritesManager().isFavourite(decl::Type::Material, "textures/common/clip"));
}

TEST_F(FavouritesTest, AddingNoneTypes)
{
    // Should always be empty
    EXPECT_TRUE(GlobalFavouritesManager().getFavourites(decl::Type::None).empty());

    // None reports always false
    EXPECT_FALSE(GlobalFavouritesManager().isFavourite(decl::Type::None, "textures/common/clip"));

    // Add clip to None category
    GlobalFavouritesManager().addFavourite(decl::Type::None, "textures/common/clip");

    // Should still be empty
    EXPECT_TRUE(GlobalFavouritesManager().getFavourites(decl::Type::None).empty());

    // Should still report as false
    EXPECT_FALSE(GlobalFavouritesManager().isFavourite(decl::Type::None, "textures/common/clip"));
}

TEST_F(FavouritesTest, RemovingNoneTypes)
{
    // Should always be empty
    EXPECT_TRUE(GlobalFavouritesManager().getFavourites(decl::Type::None).empty());

    // Remove clip to None category - it's not there, but it shouldn't crash
    GlobalFavouritesManager().removeFavourite(decl::Type::None, "textures/common/clip");

    // Should still be empty
    EXPECT_TRUE(GlobalFavouritesManager().getFavourites(decl::Type::None).empty());
}

TEST_F(FavouritesTest, RemovingNonExistent)
{
    EXPECT_TRUE(GlobalFavouritesManager().getFavourites(decl::Type::Material).empty());

    // Add caulk
    GlobalFavouritesManager().addFavourite(decl::Type::Material, "textures/common/caulk");

    EXPECT_EQ(GlobalFavouritesManager().getFavourites(decl::Type::Material).size(), 1);
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite(decl::Type::Material, "textures/common/caulk"));

    // Remove non-existent => shouldn't change anything
    GlobalFavouritesManager().removeFavourite(decl::Type::Material, "textures/doesntexist");

    EXPECT_EQ(GlobalFavouritesManager().getFavourites(decl::Type::Material).size(), 1);
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite(decl::Type::Material, "textures/common/caulk"));
}

TEST_F(FavouritesTest, AddingEmptyPaths)
{
    EXPECT_TRUE(GlobalFavouritesManager().getFavourites(decl::Type::Material).empty());

    // Add an empty string
    GlobalFavouritesManager().addFavourite(decl::Type::Material, "");

    // Should still be empty
    EXPECT_TRUE(GlobalFavouritesManager().getFavourites(decl::Type::Material).empty());

    // Empty strings always evaluate to false
    EXPECT_FALSE(GlobalFavouritesManager().isFavourite(decl::Type::Material, ""));
}

TEST_F(FavouritesTest, FavouritesArePersisted)
{
    EXPECT_TRUE(GlobalFavouritesManager().getFavourites(decl::Type::Material).empty());

    // Add caulk and clip
    GlobalFavouritesManager().addFavourite(decl::Type::Material, "textures/common/caulk");
    GlobalFavouritesManager().addFavourite(decl::Type::Material, "textures/common/clip");

    EXPECT_EQ(GlobalFavouritesManager().getFavourites(decl::Type::Material).size(), 2);
    EXPECT_EQ(GlobalFavouritesManager().getFavourites(decl::Type::Material).count("textures/common/caulk"), 1);
    EXPECT_EQ(GlobalFavouritesManager().getFavourites(decl::Type::Material).count("textures/common/clip"), 1);

    checkAfterShutdown = [&]()
    {
        settings::SettingsManager manager(_context);
        fs::path userXml = manager.getCurrentVersionSettingsFolder();
        userXml /= "user.xml";

        xml::Document doc(userXml.string());

        auto savedNodes = doc.findXPath("/user/ui/favourites/materials//favourite");

        EXPECT_EQ(savedNodes.size(), 2);

        std::set<std::string> savedFavourites;
        for (const auto& node : savedNodes)
        {
            savedFavourites.emplace(node.getAttributeValue("value"));
        }

        EXPECT_EQ(savedFavourites.count("textures/common/caulk"), 1);
        EXPECT_EQ(savedFavourites.count("textures/common/clip"), 1);
    };
}

TEST_F(FavouritesTestWithLegacyFavourites, LegacyFavouritesAreImported)
{
    // The settings in the old location in the user.xml file should have been imported
    EXPECT_FALSE(GlobalFavouritesManager().getFavourites(decl::Type::Material).empty());

    // These were present in the legacy nodes
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite(decl::Type::Material, "textures/base_trim/gotrustcol1"));
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite(decl::Type::Material, "textures/common/caulk"));
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite(decl::Type::Material, "textures/common/clip"));
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite(decl::Type::Material, "textures/common/monster_clip"));
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite(decl::Type::Material, "textures/darkmod/stone/cobblestones/blocks_mixedsize01_dark"));
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite(decl::Type::Material, "textures/darkmod/stone/cobblestones/blocks_mixedsize03_mossy"));

    // This one is already saved in the new location and should have been loaded too
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite(decl::Type::Material, "textures/common/ladder"));

    // The old node should have been automatically removed
    auto legacyNodes = GlobalRegistry().findXPath("user/ui/mediaBrowser/favourites");
    EXPECT_TRUE(legacyNodes.empty());
}

TEST_F(FavouritesTest, ChangedSignals)
{
    EXPECT_TRUE(GlobalFavouritesManager().getFavourites(decl::Type::Material).empty());

    bool signalFired = false;

    GlobalFavouritesManager().getSignalForType(decl::Type::Material).connect([&]()
    {
        signalFired = true;
    });

    // Add caulk
    GlobalFavouritesManager().addFavourite(decl::Type::Material, "textures/common/caulk");
    EXPECT_TRUE(signalFired);

    signalFired = false; // reset

    // Add the same again => shouldn't fire
    GlobalFavouritesManager().addFavourite(decl::Type::Material, "textures/common/caulk");
    EXPECT_FALSE(signalFired);

    // Remove non-existent => shouldn't fire
    GlobalFavouritesManager().removeFavourite(decl::Type::Material, "textures/doesntexist");
    EXPECT_FALSE(signalFired);

    // Add a different type => shouldn't fire
    GlobalFavouritesManager().addFavourite(decl::Type::EntityDef, "my_entity");
    EXPECT_FALSE(signalFired);

    GlobalFavouritesManager().removeFavourite(decl::Type::Material, "textures/common/caulk");
    EXPECT_TRUE(signalFired);
}

TEST_F(FavouritesTest, RequestingSignalForNoneType)
{
    bool exceptionThrown = false;

    try
    {
        GlobalFavouritesManager().getSignalForType(decl::Type::None);
    }
    catch (const std::logic_error&)
    {
        exceptionThrown = true;
    }

    EXPECT_TRUE(exceptionThrown);
}

}
