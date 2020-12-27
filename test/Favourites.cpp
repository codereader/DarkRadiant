#include "RadiantTest.h"

#include "ifavourites.h"

namespace test
{

using FavouritesTest = RadiantTest;

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

}
