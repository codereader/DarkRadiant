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

TEST_F(AseImportTest, UVOffsetKeyword)
{
    // Test Cube doesn't have any offset
    auto model = GlobalModelCache().getModel("models/ase/testcube.ase");
    EXPECT_EQ(model->getSurfaceCount(), 1);

    EXPECT_NEAR(model->getSurface(0).getVertex(0).texcoord.x(), 0, 1e-5);
    EXPECT_NEAR(model->getSurface(0).getVertex(0).texcoord.y(), 1, 1e-5);

    EXPECT_NEAR(model->getSurface(0).getVertex(1).texcoord.x(), 0, 1e-5);
    EXPECT_NEAR(model->getSurface(0).getVertex(1).texcoord.y(), 0, 1e-5);

    EXPECT_NEAR(model->getSurface(0).getVertex(2).texcoord.x(), 1, 1e-5);
    EXPECT_NEAR(model->getSurface(0).getVertex(2).texcoord.y(), 1, 1e-5);

    // Model has UVW_U_OFFSET == 0.5 and UVW_V_OFFSET == 0.3
    model = GlobalModelCache().getModel("models/ase/testcube_uv_offset.ase");
    EXPECT_EQ(model->getSurfaceCount(), 1);

    // U_OFFSET is negated and applied => -0.5
    EXPECT_NEAR(model->getSurface(0).getVertex(0).texcoord.x(), -0.5, 1e-5);
    EXPECT_NEAR(model->getSurface(0).getVertex(0).texcoord.y(), 1.3, 1e-5);

    EXPECT_NEAR(model->getSurface(0).getVertex(1).texcoord.x(), -0.5, 1e-5);
    EXPECT_NEAR(model->getSurface(0).getVertex(1).texcoord.y(), 0.3, 1e-5);

    EXPECT_NEAR(model->getSurface(0).getVertex(2).texcoord.x(), 0.5, 1e-5);
    EXPECT_NEAR(model->getSurface(0).getVertex(2).texcoord.y(), 1.3, 1e-5);
}

TEST_F(AseImportTest, UVTilingKeyword)
{
    // Default testcube.ase is already tested in the UVOffsetKeyword test

    // Model has UVW_U_TILING == 2 and UVW_V_TILING == 3
    auto model = GlobalModelCache().getModel("models/ase/testcube_uv_tiling.ase");
    EXPECT_EQ(model->getSurfaceCount(), 1);

    EXPECT_NEAR(model->getSurface(0).getVertex(0).texcoord.x(), 0, 1e-5);
    EXPECT_NEAR(model->getSurface(0).getVertex(0).texcoord.y(), 3, 1e-5);

    EXPECT_NEAR(model->getSurface(0).getVertex(1).texcoord.x(), 0, 1e-5);
    EXPECT_NEAR(model->getSurface(0).getVertex(1).texcoord.y(), 0, 1e-5);

    EXPECT_NEAR(model->getSurface(0).getVertex(2).texcoord.x(), 2, 1e-5);
    EXPECT_NEAR(model->getSurface(0).getVertex(2).texcoord.y(), 3, 1e-5);
}

TEST_F(AseImportTest, UVAngleKeyword)
{
    // Default testcube.ase is already tested in the UVOffsetKeyword test

    // Model has UVW_ANGLE == 1.570796 (pi/2)
    auto model = GlobalModelCache().getModel("models/ase/testcube_uv_angle.ase");
    EXPECT_EQ(model->getSurfaceCount(), 1);

    auto sinValue = sin(math::PI / 2);
    auto cosValue = cos(math::PI / 2);

    EXPECT_NEAR(model->getSurface(0).getVertex(0).texcoord.x(), 0 * cosValue + 1 * sinValue, 1e-5);
    EXPECT_NEAR(model->getSurface(0).getVertex(0).texcoord.y(), 0 * -sinValue + 1 * cosValue, 1e-5);

    EXPECT_NEAR(model->getSurface(0).getVertex(1).texcoord.x(), 0 * cosValue + 0 * sinValue, 1e-5);
    EXPECT_NEAR(model->getSurface(0).getVertex(1).texcoord.y(), 0 * -sinValue + 0 * cosValue, 1e-5);

    EXPECT_NEAR(model->getSurface(0).getVertex(2).texcoord.x(), 1 * cosValue + 1 * sinValue, 1e-5);
    EXPECT_NEAR(model->getSurface(0).getVertex(2).texcoord.y(), 1 * -sinValue + 1 * cosValue, 1e-5);
}

bool surfaceHasVertexWith(const model::IModelSurface& surface, 
    const std::function<bool(const ArbitraryMeshVertex& vertex)>& predicate)
{
    bool found = false;

    for (auto i = 0; i < surface.getNumVertices(); ++i)
    {
        if (predicate(surface.getVertex(i)))
        {
            found = true;
            break;
        }
    }

    return found;
}

void expectVertexWithNormal(const model::IModelSurface& surface, const Vertex3f& vertex, const Normal3f& normal)
{
    EXPECT_TRUE(surfaceHasVertexWith(surface, [&](const ArbitraryMeshVertex& v)->bool
    {
        return v.vertex.isEqual(vertex, 1e-5) && v.normal.isEqual(normal, 1e-5);
    })) << "Could not find a vertex with xyz = " << vertex << " and normal " << normal;
}

TEST_F(AseImportTest, VertexNormals)
{
    // Test Cube doesn't have any offset
    auto model = GlobalModelCache().getModel("models/ase/testcube.ase");
    EXPECT_EQ(model->getSurfaceCount(), 1);

    // Check for a few specific vertex/normal combinations
    expectVertexWithNormal(model->getSurface(0), Vertex3f(-16, -16, 16), Normal3f(-1, 0, 0));
    expectVertexWithNormal(model->getSurface(0), Vertex3f(-16, -16, -16), Normal3f(-1, 0, 0));
    expectVertexWithNormal(model->getSurface(0), Vertex3f(-16, 16, -16), Normal3f(-1, 0, 0));

    expectVertexWithNormal(model->getSurface(0), Vertex3f(-16, 16, 16), Normal3f(0, 1, 0));
    expectVertexWithNormal(model->getSurface(0), Vertex3f(-16, 16, -16), Normal3f(0, 1, 0));
    expectVertexWithNormal(model->getSurface(0), Vertex3f(16, 16, -16), Normal3f(0, 1, 0));

    expectVertexWithNormal(model->getSurface(0), Vertex3f(16, 16, 16), Normal3f(1, 0, 0));
    expectVertexWithNormal(model->getSurface(0), Vertex3f(16, 16, -16), Normal3f(1, 0, 0));
    expectVertexWithNormal(model->getSurface(0), Vertex3f(16, -16, -16), Normal3f(1, 0, 0));

    expectVertexWithNormal(model->getSurface(0), Vertex3f(16, -16, 16), Normal3f(0, -1, 0));
    expectVertexWithNormal(model->getSurface(0), Vertex3f(16, -16, -16), Normal3f(0, -1, 0));
    expectVertexWithNormal(model->getSurface(0), Vertex3f(-16, -16, -16), Normal3f(0, -1, 0));

    expectVertexWithNormal(model->getSurface(0), Vertex3f(16, 16, -16), Normal3f(0, 0, -1));
    expectVertexWithNormal(model->getSurface(0), Vertex3f(-16, 16, -16), Normal3f(0, 0, -1));
    expectVertexWithNormal(model->getSurface(0), Vertex3f(-16, -16, -16), Normal3f(0, 0, -1));

    expectVertexWithNormal(model->getSurface(0), Vertex3f(-16, 16, 16), Normal3f(0, 0, 1));
    expectVertexWithNormal(model->getSurface(0), Vertex3f(16, 16, 16), Normal3f(0, 0, 1));
    expectVertexWithNormal(model->getSurface(0), Vertex3f(16, -16, 16), Normal3f(0, 0, 1));
}

}
