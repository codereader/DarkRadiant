#include "RadiantTest.h"

#include "imodelsurface.h"
#include "imodelcache.h"

namespace test
{

using ModelTest = RadiantTest;
using AseImportTest = ModelTest;

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
TEST_F(AseImportTest, BitmapFieldPreferredOverMaterialName)
{
    auto model = GlobalModelCache().getModel("models/missing_texture.ase");
    EXPECT_TRUE(model);

    EXPECT_EQ(model->getSurfaceCount(), 1);
    EXPECT_EQ(model->getSurface(0).getDefaultMaterial(), "textures/doesnt_exist");
}

TEST_F(AseImportTest, MaterialNameIsExtractedAndNormalised)
{
    auto model = GlobalModelCache().getModel("models/ase/tiles_with_shared_vertex_and_colour.ase");
    EXPECT_TRUE(model);

    // *BITMAP "\\base\textures\common\clip" is converted to the normalised name
    EXPECT_EQ(model->getSurface(0).getDefaultMaterial(), "textures/common/clip");
}

TEST_F(AseImportTest, MaterialAndSurfaceCount)
{
    auto model = GlobalModelCache().getModel("models/ase/tiles_two_materials.ase");
    EXPECT_TRUE(model);

    // This model has several *GEOMOBJECTs, but only two distinct materials => two surfaces
    EXPECT_EQ(model->getSurfaceCount(), 2);
    EXPECT_EQ(model->getSurface(0).getDefaultMaterial(), "material");
    EXPECT_EQ(model->getSurface(1).getDefaultMaterial(), "tork");

    // Only 1 material, only 1 surface
    model = GlobalModelCache().getModel("models/ase/tiles_with_shared_vertex_and_colour.ase");
    EXPECT_TRUE(model);

    EXPECT_EQ(model->getSurfaceCount(), 1);
    EXPECT_EQ(model->getSurface(0).getDefaultMaterial(), "textures/common/clip");
}

TEST_F(AseImportTest, VertexAndTriangleCount)
{
    auto model = GlobalModelCache().getModel("models/ase/single_triangle.ase");
    EXPECT_EQ(model->getSurfaceCount(), 1);
    EXPECT_EQ(model->getSurface(0).getNumVertices(), 3);
    EXPECT_EQ(model->getSurface(0).getNumTriangles(), 1);

    // Model has 8 vertices, but they have different normals/UVs due to being shared among the sides
    // so these 8 prototypes should be all instantiated to a total of 24 vertices
    model = GlobalModelCache().getModel("models/ase/testcube.ase");
    EXPECT_EQ(model->getSurfaceCount(), 1);
    EXPECT_EQ(model->getSurface(0).getNumVertices(), 24);
    EXPECT_EQ(model->getSurface(0).getNumTriangles(), 12);

    model = GlobalModelCache().getModel("models/ase/testsphere.ase");
    EXPECT_EQ(model->getSurfaceCount(), 1);
    EXPECT_EQ(model->getSurface(0).getNumVertices(), 962);
    EXPECT_EQ(model->getSurface(0).getNumTriangles(), 960);

    // Model has 6 separated quad objects (total: 12 tris), 1 material, no dupes
    model = GlobalModelCache().getModel("models/ase/exploded_cube.ase");
    EXPECT_EQ(model->getSurfaceCount(), 1);
    EXPECT_EQ(model->getSurface(0).getNumVertices(), 24);
    EXPECT_EQ(model->getSurface(0).getNumTriangles(), 12);

    model = GlobalModelCache().getModel("models/ase/separated_tiles.ase");
    EXPECT_EQ(model->getSurfaceCount(), 1);
    EXPECT_EQ(model->getSurface(0).getNumVertices(), 16);
    EXPECT_EQ(model->getSurface(0).getNumTriangles(), 8);

    model = GlobalModelCache().getModel("models/ase/tiles_with_shared_vertex.ase");
    EXPECT_EQ(model->getSurfaceCount(), 1);
    EXPECT_EQ(model->getSurface(0).getNumVertices(), 13);
    EXPECT_EQ(model->getSurface(0).getNumTriangles(), 8);

    model = GlobalModelCache().getModel("models/ase/tiles_with_shared_vertex_and_colour.ase");
    EXPECT_EQ(model->getSurfaceCount(), 1);
    EXPECT_EQ(model->getSurface(0).getNumVertices(), 13);
    EXPECT_EQ(model->getSurface(0).getNumTriangles(), 8);

    model = GlobalModelCache().getModel("models/ase/tiles.ase");
    EXPECT_EQ(model->getSurfaceCount(), 1);
    EXPECT_EQ(model->getSurface(0).getNumVertices(), 4);
    EXPECT_EQ(model->getSurface(0).getNumTriangles(), 2);

    model = GlobalModelCache().getModel("models/ase/tiles_two_materials.ase");
    EXPECT_EQ(model->getSurfaceCount(), 2);
    EXPECT_EQ(model->getSurface(0).getNumVertices(), 8);
    EXPECT_EQ(model->getSurface(0).getNumTriangles(), 4);
    EXPECT_EQ(model->getSurface(1).getNumVertices(), 8);
    EXPECT_EQ(model->getSurface(1).getNumTriangles(), 4);

    model = GlobalModelCache().getModel("models/ase/merged_cube.ase");
    EXPECT_EQ(model->getSurfaceCount(), 1);
    EXPECT_EQ(model->getSurface(0).getNumVertices(), 24);
    EXPECT_EQ(model->getSurface(0).getNumTriangles(), 12);
}

TEST_F(AseImportTest, TriangleWindingCW)
{
    // Triangle windings in DR need to be CW, whereas ASE stores them in CCW order
    // This test assumes that the model indices are translated correctly to have CW windings
    auto model = GlobalModelCache().getModel("models/ase/single_triangle.ase");
    EXPECT_EQ(model->getSurfaceCount(), 1);

    EXPECT_NO_THROW(dynamic_cast<const model::IIndexedModelSurface&>(model->getSurface(0)));
    const auto& surface = static_cast<const model::IIndexedModelSurface&>(model->getSurface(0));
    
    const auto& vertices = surface.getVertexArray();
    const auto& indices = surface.getIndexArray();
    EXPECT_EQ(vertices.size(), 3);
    EXPECT_EQ(indices.size(), 3);

    // Take the cross-product of the first two vectors of the winding
    const auto& a = vertices[indices[0]].vertex;
    const auto& b = vertices[indices[1]].vertex;
    const auto& c = vertices[indices[2]].vertex;

    auto normal = (b - a).crossProduct(c - b).getNormalised();
    
    // We know the triangle in the ASE file is facing upwards, 
    // For CW order, the cross-product will point in the opposite direction 
    // of the normal, i.e. downwards
    EXPECT_NEAR(normal.z(), -1.0, 1e-4);
}

}
