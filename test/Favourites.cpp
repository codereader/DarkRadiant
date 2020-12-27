#include "RadiantTest.h"

#include "ifavourites.h"
#include "iregistry.h"

namespace test
{

class FavouritesTest :
    public RadiantTest
{
protected:
    void copyUserXmlFileToSettingsPath(const std::string& userXmlFile)
    {
        fs::path sourcePath = _context.getTestResourcePath();
        sourcePath /= "settings/";
        sourcePath /= userXmlFile;

        fs::path targetPath = _context.getSettingsPath();
        targetPath /= "user.xml";

        fs::remove(targetPath);
        fs::copy(sourcePath, targetPath);
    }
};

class FavouritesTestWithLegacyFavourites :
    public FavouritesTest
{
public:
    void SetUp() override
    {
        copyUserXmlFileToSettingsPath("old_mediabrowser_favourites.xml");

        RadiantTest::SetUp();
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

}
