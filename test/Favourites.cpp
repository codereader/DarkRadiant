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
        copyUserXmlFileToSettingsPath("old_favourites.xml");
    }
};

TEST_F(FavouritesTest, AddingAndRemovingFavourites)
{
    EXPECT_TRUE(GlobalFavouritesManager().getFavourites("material").empty());

    // Add caulk
    GlobalFavouritesManager().addFavourite("material", "textures/common/caulk");

    EXPECT_EQ(GlobalFavouritesManager().getFavourites("material").size(), 1);
    EXPECT_EQ(GlobalFavouritesManager().getFavourites("material").count("textures/common/caulk"), 1);
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite("material", "textures/common/caulk"));
    EXPECT_FALSE(GlobalFavouritesManager().isFavourite("material", "textures/common/clip"));

    // Add clip
    GlobalFavouritesManager().addFavourite("material", "textures/common/clip");

    EXPECT_EQ(GlobalFavouritesManager().getFavourites("material").size(), 2);
    EXPECT_EQ(GlobalFavouritesManager().getFavourites("material").count("textures/common/caulk"), 1);
    EXPECT_EQ(GlobalFavouritesManager().getFavourites("material").count("textures/common/clip"), 1);
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite("material", "textures/common/caulk"));
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite("material", "textures/common/clip"));

    // Remove caulk
    GlobalFavouritesManager().removeFavourite("material", "textures/common/caulk");

    EXPECT_EQ(GlobalFavouritesManager().getFavourites("material").size(), 1);
    EXPECT_EQ(GlobalFavouritesManager().getFavourites("material").count("textures/common/clip"), 1);
    EXPECT_FALSE(GlobalFavouritesManager().isFavourite("material", "textures/common/caulk"));
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite("material", "textures/common/clip"));

    // Remove clip
    GlobalFavouritesManager().removeFavourite("material", "textures/common/clip");

    EXPECT_EQ(GlobalFavouritesManager().getFavourites("material").size(), 0);
    EXPECT_FALSE(GlobalFavouritesManager().isFavourite("material", "textures/common/caulk"));
    EXPECT_FALSE(GlobalFavouritesManager().isFavourite("material", "textures/common/clip"));
}

TEST_F(FavouritesTest, CaseSensitivity)
{
    EXPECT_TRUE(GlobalFavouritesManager().getFavourites("material").empty());
    EXPECT_TRUE(GlobalFavouritesManager().getFavourites("MATErial").empty());

    // Add caulk
    GlobalFavouritesManager().addFavourite("material", "textures/common/caulk");

    EXPECT_EQ(GlobalFavouritesManager().getFavourites("material").size(), 1);
    EXPECT_EQ(GlobalFavouritesManager().getFavourites("MATErial").size(), 1);
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite("material", "textures/common/caulk"));
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite("MATErial", "textures/common/caulk"));

    // Remove caulk
    GlobalFavouritesManager().removeFavourite("MATErial", "textures/common/caulk");

    EXPECT_EQ(GlobalFavouritesManager().getFavourites("material").size(), 0);
    EXPECT_EQ(GlobalFavouritesManager().getFavourites("MATErial").size(), 0);
}

TEST_F(FavouritesTest, AddingNoneTypes)
{
    // Should always be empty
    EXPECT_TRUE(GlobalFavouritesManager().getFavourites("").empty());

    // None reports always false
    EXPECT_FALSE(GlobalFavouritesManager().isFavourite("", "textures/common/clip"));

    // Add clip to empty category
    GlobalFavouritesManager().addFavourite("", "textures/common/clip");

    // Should still be empty
    EXPECT_TRUE(GlobalFavouritesManager().getFavourites("").empty());

    // Should still report as false
    EXPECT_FALSE(GlobalFavouritesManager().isFavourite("", "textures/common/clip"));
}

TEST_F(FavouritesTest, RemovingNoneTypes)
{
    // Should always be empty
    EXPECT_TRUE(GlobalFavouritesManager().getFavourites("").empty());

    // Remove clip to empty category - it's not there, but it shouldn't crash
    GlobalFavouritesManager().removeFavourite("", "textures/common/clip");

    // Should still be empty
    EXPECT_TRUE(GlobalFavouritesManager().getFavourites("").empty());
}

TEST_F(FavouritesTest, RemovingNonExistent)
{
    EXPECT_TRUE(GlobalFavouritesManager().getFavourites("material").empty());

    // Add caulk
    GlobalFavouritesManager().addFavourite("material", "textures/common/caulk");

    EXPECT_EQ(GlobalFavouritesManager().getFavourites("material").size(), 1);
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite("material", "textures/common/caulk"));

    // Remove non-existent => shouldn't change anything
    GlobalFavouritesManager().removeFavourite("material", "textures/doesntexist");

    EXPECT_EQ(GlobalFavouritesManager().getFavourites("material").size(), 1);
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite("material", "textures/common/caulk"));
}

TEST_F(FavouritesTest, AddingEmptyPaths)
{
    EXPECT_TRUE(GlobalFavouritesManager().getFavourites("material").empty());

    // Add an empty string
    GlobalFavouritesManager().addFavourite("material", "");

    // Should still be empty
    EXPECT_TRUE(GlobalFavouritesManager().getFavourites("material").empty());

    // Empty strings always evaluate to false
    EXPECT_FALSE(GlobalFavouritesManager().isFavourite("material", ""));
}

TEST_F(FavouritesTest, FavouritesArePersisted)
{
    EXPECT_TRUE(GlobalFavouritesManager().getFavourites("material").empty());

    // Add caulk and clip
    GlobalFavouritesManager().addFavourite("material", "textures/common/caulk");
    GlobalFavouritesManager().addFavourite("material", "textures/common/clip");

    EXPECT_EQ(GlobalFavouritesManager().getFavourites("material").size(), 2);
    EXPECT_EQ(GlobalFavouritesManager().getFavourites("material").count("textures/common/caulk"), 1);
    EXPECT_EQ(GlobalFavouritesManager().getFavourites("material").count("textures/common/clip"), 1);

    checkAfterShutdown = [&]()
    {
        settings::SettingsManager manager(_context);
        fs::path userXml = manager.getCurrentVersionSettingsFolder();
        userXml /= "user.xml";

        xml::Document doc(userXml.string());

        auto savedNodes = doc.findXPath("/user/ui/favourites/material//favourite");

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
    EXPECT_FALSE(GlobalFavouritesManager().getFavourites("material").empty());

    // These were present in the mediabrowser legacy nodes
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite("material", "textures/base_trim/gotrustcol1"));
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite("material", "textures/common/caulk"));
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite("material", "textures/common/clip"));
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite("material", "textures/common/monster_clip"));
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite("material", "textures/darkmod/stone/cobblestones/blocks_mixedsize01_dark"));
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite("material", "textures/darkmod/stone/cobblestones/blocks_mixedsize03_mossy"));

    // These were declared in the decl-typed nodes
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite("material", "textures/common/ladder"));
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite("entityDef", "atdm:ai_proguard"));
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite("soundShader", "clock_tick01"));
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite("soundShader", "clock_tick02"));
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite("model", "bush02.lwo"));
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite("model", "bush01.lwo"));
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite("particle", "heavy_rain"));
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite("particle", "barrelfire"));

    // This one is already saved in the new location and should have been loaded too
    EXPECT_TRUE(GlobalFavouritesManager().isFavourite("material", "textures/some/weird/name"));

    // The old nodes should have been automatically removed
    EXPECT_TRUE(GlobalRegistry().findXPath("user/ui/mediaBrowser/favourites").empty());
    EXPECT_TRUE(GlobalRegistry().findXPath("user/ui/favourites/materials").empty());
    EXPECT_TRUE(GlobalRegistry().findXPath("user/ui/favourites/entityDefs").empty());
    EXPECT_TRUE(GlobalRegistry().findXPath("user/ui/favourites/soundShaders").empty());
    EXPECT_TRUE(GlobalRegistry().findXPath("user/ui/favourites/models").empty());
    EXPECT_TRUE(GlobalRegistry().findXPath("user/ui/favourites/particles").empty());
}

TEST_F(FavouritesTest, ChangedSignals)
{
    EXPECT_TRUE(GlobalFavouritesManager().getFavourites("material").empty());

    bool signalFired = false;

    GlobalFavouritesManager().getSignalForType("material").connect([&]()
    {
        signalFired = true;
    });

    // Add caulk
    GlobalFavouritesManager().addFavourite("material", "textures/common/caulk");
    EXPECT_TRUE(signalFired);

    signalFired = false; // reset

    // Add the same again => shouldn't fire
    GlobalFavouritesManager().addFavourite("material", "textures/common/caulk");
    EXPECT_FALSE(signalFired);

    // Remove non-existent => shouldn't fire
    GlobalFavouritesManager().removeFavourite("material", "textures/doesntexist");
    EXPECT_FALSE(signalFired);

    // Add a different type => shouldn't fire
    GlobalFavouritesManager().addFavourite("entitydef", "my_entity");
    EXPECT_FALSE(signalFired);

    GlobalFavouritesManager().removeFavourite("material", "textures/common/caulk");
    EXPECT_TRUE(signalFired);
}

TEST_F(FavouritesTest, RequestingSignalForNoneType)
{
    EXPECT_THROW(GlobalFavouritesManager().getSignalForType(""), std::invalid_argument);
}

}
