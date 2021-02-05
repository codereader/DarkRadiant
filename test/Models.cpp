#include "RadiantTest.h"

#include "imodelcache.h"

namespace test
{

using ModelTest = RadiantTest;

TEST_F(ModelTest, LwoPolyCount)
{
    auto model = GlobalModelCache().getModel("models/darkmod/test/unit_cube.lwo");
    EXPECT_TRUE(model);

    // The unit cube should have 12 polys (6 quads = 12 tris)
    EXPECT_EQ(model->getPolyCount(), 12);
}

TEST_F(ModelTest, AsePolyCount)
{
    auto model = GlobalModelCache().getModel("models/darkmod/test/unit_cube.ase");
    EXPECT_TRUE(model);

    // The unit cube should have 12 polys (6 quads = 12 tris)
    EXPECT_EQ(model->getPolyCount(), 12);
}

// #4644: If the *BITMAP material cannot be resolved, the code should not fall back to *MATERIAL_NAME (in TDM/idTech4)
TEST_F(ModelTest, BitmapFieldPreferredOverMaterialName)
{
    auto model = GlobalModelCache().getModel("models/missing_texture.ase");
    EXPECT_TRUE(model);

    EXPECT_EQ(model->getSurfaceCount(), 1);
    EXPECT_EQ(model->getSurface(0).getDefaultMaterial(), "textures/doesnt_exist");
}

}
