#include "RadiantTest.h"

#include <unordered_set>
#include "imodelsurface.h"
#include "imodelcache.h"
#include "scenelib.h"
#include "algorithm/Entity.h"
#include "algorithm/Scene.h"

#include "render/VertexHashing.h"

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

    model = GlobalModelCache().getModel("models/ase/gauge_needle.ase");
    EXPECT_EQ(model->getSurfaceCount(), 1);
    EXPECT_EQ(model->getSurface(0).getNumVertices(), 14);
    EXPECT_EQ(model->getSurface(0).getNumTriangles(), 11);
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

    auto normal = (b - a).cross(c - b).getNormalised();

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
    const std::function<bool(const MeshVertex& vertex)>& predicate)
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

void expectVertexWithNormal(const model::IModelSurface& surface, const Vertex3& vertex, const Normal3& normal)
{
    EXPECT_TRUE(surfaceHasVertexWith(surface, [&](const MeshVertex& v)->bool
    {
        return math::isNear(v.vertex, vertex, render::VertexEpsilon) && v.normal.dot(normal) > 1.0 - render::NormalEpsilon;
    })) << "Could not find a vertex with xyz = " << vertex << " and normal " << normal;
}

TEST_F(AseImportTest, VertexNormals)
{
    auto model = GlobalModelCache().getModel("models/ase/testcube.ase");
    EXPECT_EQ(model->getSurfaceCount(), 1);

    // Check for a few specific vertex/normal combinations
    expectVertexWithNormal(model->getSurface(0), Vertex3(-16, -16, 16), Normal3(-1, 0, 0));
    expectVertexWithNormal(model->getSurface(0), Vertex3(-16, -16, -16), Normal3(-1, 0, 0));
    expectVertexWithNormal(model->getSurface(0), Vertex3(-16, 16, -16), Normal3(-1, 0, 0));

    expectVertexWithNormal(model->getSurface(0), Vertex3(-16, 16, 16), Normal3(0, 1, 0));
    expectVertexWithNormal(model->getSurface(0), Vertex3(-16, 16, -16), Normal3(0, 1, 0));
    expectVertexWithNormal(model->getSurface(0), Vertex3(16, 16, -16), Normal3(0, 1, 0));

    expectVertexWithNormal(model->getSurface(0), Vertex3(16, 16, 16), Normal3(1, 0, 0));
    expectVertexWithNormal(model->getSurface(0), Vertex3(16, 16, -16), Normal3(1, 0, 0));
    expectVertexWithNormal(model->getSurface(0), Vertex3(16, -16, -16), Normal3(1, 0, 0));

    expectVertexWithNormal(model->getSurface(0), Vertex3(16, -16, 16), Normal3(0, -1, 0));
    expectVertexWithNormal(model->getSurface(0), Vertex3(16, -16, -16), Normal3(0, -1, 0));
    expectVertexWithNormal(model->getSurface(0), Vertex3(-16, -16, -16), Normal3(0, -1, 0));

    expectVertexWithNormal(model->getSurface(0), Vertex3(16, 16, -16), Normal3(0, 0, -1));
    expectVertexWithNormal(model->getSurface(0), Vertex3(-16, 16, -16), Normal3(0, 0, -1));
    expectVertexWithNormal(model->getSurface(0), Vertex3(-16, -16, -16), Normal3(0, 0, -1));

    expectVertexWithNormal(model->getSurface(0), Vertex3(-16, 16, 16), Normal3(0, 0, 1));
    expectVertexWithNormal(model->getSurface(0), Vertex3(16, 16, 16), Normal3(0, 0, 1));
    expectVertexWithNormal(model->getSurface(0), Vertex3(16, -16, 16), Normal3(0, 0, 1));
}

void expectVertexWithColour(const model::IModelSurface& surface, const Vertex3& vertex, const Vector4& colour)
{
    EXPECT_TRUE(surfaceHasVertexWith(surface, [&](const MeshVertex& v)->bool
    {
        return math::isNear(v.vertex, vertex, render::VertexEpsilon) && math::isNear(v.colour, colour, render::VertexEpsilon);
    })) << "Could not find a vertex with xyz = " << vertex << " and colour " << colour;
}

TEST_F(AseImportTest, VertexColours)
{
    auto model = GlobalModelCache().getModel("models/ase/tiles_with_shared_vertex_and_colour.ase");
    EXPECT_EQ(model->getSurfaceCount(), 1);

    // Check for a few specific vertex/colour combinations
    expectVertexWithColour(model->getSurface(0), Vertex3(56, 56, 2), Vector4(0, 0, 0, 1));
    expectVertexWithColour(model->getSurface(0), Vertex3(56, 18, 2), Vector4(0, 0, 0, 1));
    expectVertexWithColour(model->getSurface(0), Vertex3(19, 18, 2), Vector4(0.9882, 0.9882, 0.9882, 1));
    expectVertexWithColour(model->getSurface(0), Vertex3(19, 56, 2), Vector4(1, 1, 1, 1));
    expectVertexWithColour(model->getSurface(0), Vertex3(-19, -19, 2), Vector4(0, 0, 0, 1));
    expectVertexWithColour(model->getSurface(0), Vertex3(19, -19, 2), Vector4(0, 0, 0, 1));
    expectVertexWithColour(model->getSurface(0), Vertex3(-19, 56, 2), Vector4(0, 0, 0, 1));
    expectVertexWithColour(model->getSurface(0), Vertex3(19, 56, 2), Vector4(1, 1, 1, 1));
    expectVertexWithColour(model->getSurface(0), Vertex3(-19, 18, 2), Vector4(0.9216, 0.9216, 0.9216, 1));
    expectVertexWithColour(model->getSurface(0), Vertex3(56, -19, 2), Vector4(0.7373, 0.7373, 0.7373, 1));
    expectVertexWithColour(model->getSurface(0), Vertex3(19, -19, 2), Vector4(0, 0, 0, 1));
}

// Tests the NODE_TM transform application to vertex normals
TEST_F(AseImportTest, VertexNormalTransformation)
{
    auto model = GlobalModelCache().getModel("models/ase/gauge_needle.ase");
    EXPECT_EQ(model->getSurfaceCount(), 1);

    // Check for a few specific vertex/colour combinations (values taken directly from the TDM parse result)
    expectVertexWithNormal(model->getSurface(0), Vertex3(-0.140799999, -0.745599985, 0.125799999), Normal3(0, -1, 0));
    expectVertexWithNormal(model->getSurface(0), Vertex3(0.188199997, -0.745599985, 0.125900000), Normal3(0, -1, 0));
    expectVertexWithNormal(model->getSurface(0), Vertex3(-0.0364999995, -0.745599985, 0.0203000009), Normal3(0, -1, 0));
    expectVertexWithNormal(model->getSurface(0), Vertex3(0.0839999989, -0.745599985, 0.0203000009), Normal3(0, -1, 0));
    expectVertexWithNormal(model->getSurface(0), Vertex3(-0.140900001, -0.745599985, 0.331900001), Normal3(0.000499708927, -0.999999583, 0.000800181471));
    expectVertexWithNormal(model->getSurface(0), Vertex3(0.188199997, -0.745599985, 0.331999987), Normal3(-3.27272573e-07, -0.999999583, 0.000899999577));
    expectVertexWithNormal(model->getSurface(0), Vertex3(-0.0218000002, -0.744899988, 2.23850012), Normal3(0.0139053408, -0.999789357, -0.0150947841));
    expectVertexWithNormal(model->getSurface(0), Vertex3(0.0679000020, -0.742100000, 2.23850012), Normal3(0.0298263635, -0.996775806, -0.0744873509));
    expectVertexWithNormal(model->getSurface(0), Vertex3(-0.133800000, -0.745599985, 2.29660010), Normal3(1.23635652e-06, -0.999994278, -0.00339998049));
    expectVertexWithNormal(model->getSurface(0), Vertex3(0.179800004, -0.745599985, 2.29670000), Normal3(0.00370575022, -0.999861956, -0.0161980372));
    expectVertexWithNormal(model->getSurface(0), Vertex3(0.0229000002, -0.745599985, 2.69810009), Normal3(0, -1, 0));
    expectVertexWithNormal(model->getSurface(0), Vertex3(-0.0218000002, -0.744899988, 2.23850012), Normal3(-0.869799972, 0.00000000, -0.493404597));
    expectVertexWithNormal(model->getSurface(0), Vertex3(-0.133800000, -0.745599985, 2.29660010), Normal3(-0.460478902, 0.00000000, -0.887670696));
    expectVertexWithNormal(model->getSurface(0), Vertex3(-0.140900001, -0.745599985, 0.331900001), Normal3(-0.998054981, -0.00000000, 0.0623391047));
}


TEST_F(AseImportTest, VertexHashFunction)
{
    // Construct two mesh vertices which should be considered equal
    MeshVertex vertex1(Vertex3(-0.0218, -0.7449, 2.2385), Normal3(-0.8698, 0, -0.493405), 
        TexCoord2f(0.9808, 0.8198), Vector3(1, 1, 1));

    MeshVertex vertex2(Vertex3(-0.0218, -0.7434, 2.2385), Normal3(-0.872, 0, -0.489505), 
        TexCoord2f(0.9808, 0.8198), Vector3(1, 1, 1));

    // Construct a that is differing in the normal part
    MeshVertex vertex3(Vertex3(-0.0218, -0.7434, 2.2385), Normal3(-1, 0, 0),
        TexCoord2f(0.9808, 0.8198), Vector3(1, 1, 1));

    // Check the hash behaviour
    std::hash<MeshVertex> hasher;
    EXPECT_EQ(hasher(vertex1), hasher(vertex2));
    EXPECT_EQ(hasher(vertex1), hasher(vertex3));

    std::equal_to<MeshVertex> equalityComparer;
    EXPECT_TRUE(equalityComparer(vertex1, vertex2));
    EXPECT_FALSE(equalityComparer(vertex1, vertex3));

    // With the included hash specialisations from render/VertexHashing.h, the two vertices should be considered equal
    std::unordered_set<MeshVertex> set;

    // Insert the first vertex
    EXPECT_TRUE(set.insert(vertex1).second);

    // Inserting the second vertex should fail
    EXPECT_FALSE(set.insert(vertex2).second);

    // Inserting the third vertex should succeed
    EXPECT_TRUE(set.insert(vertex3).second);
}

// #5590: ASE models with a misleading *MATERIAL_COUNT are still loaded successfully by the in-game importer
TEST_F(AseImportTest, LoadAseWithWrongMaterialCount)
{
    auto model = GlobalModelCache().getModel("models/darkmod/test/cube_wrong_material_count.ase");
    EXPECT_EQ(model->getSurfaceCount(), 4);

    std::set<std::string> materials;

    for (auto i = 0; i < model->getSurfaceCount(); ++i)
    {
        materials.insert(model->getSurface(i).getDefaultMaterial());
    }

    EXPECT_EQ(materials.count("material01"), 1) << "material01 not found";
    EXPECT_EQ(materials.count("material02"), 1) << "material02 not found";
    EXPECT_EQ(materials.count("material03"), 1) << "material03 not found";
    EXPECT_EQ(materials.count("material04"), 1) << "material04 not found";
}

TEST_F(AseImportTest, ParseMeshFaceWithoutSmoothing)
{
    // This model contains *MESH_FACE lines without *MESH_SMOOTHING, the parser needs to be able to deal with that
    auto model = GlobalModelCache().getModel("models/ase/testcube_no_smoothing_in_mesh_face.ase");

    // Model should be loaded successfully
    EXPECT_TRUE(model);

    if (model)
    {
        EXPECT_EQ(model->getSurfaceCount(), 1);
        EXPECT_EQ(model->getSurface(0).getNumVertices(), 24);
        EXPECT_EQ(model->getSurface(0).getNumTriangles(), 12);
    }
}

TEST_F(AseImportTest, ParseMeshFaceWithoutABBCCA)
{
    // This model contains *MESH_FACE lines without AB, BC, CA, the parser needs to be able to deal with that
    auto model = GlobalModelCache().getModel("models/ase/testcube_no_ab_bc_ca_in_mesh_face.ase");

    // Model should be loaded successfully
    EXPECT_TRUE(model);

    if (model)
    {
        EXPECT_EQ(model->getSurfaceCount(), 1);
        EXPECT_EQ(model->getSurface(0).getNumVertices(), 24);
        EXPECT_EQ(model->getSurface(0).getNumTriangles(), 12);
    }
}

TEST_F(AseImportTest, ParseGeomObjectWithoutMaterialRef)
{
    // This model contains a *GEOMOBJECT without any *MATERIAL_REF the parser needs to be able to deal with that
    auto model = GlobalModelCache().getModel("models/ase/testcube_without_material_ref.ase");

    // Model should be loaded successfully
    EXPECT_TRUE(model);

    if (model)
    {
        EXPECT_EQ(model->getSurfaceCount(), 1);
        EXPECT_EQ(model->getSurface(0).getDefaultMaterial(), "textures/darkmod/stone/flat/tiles_rough_grey");
        EXPECT_EQ(model->getSurface(0).getNumVertices(), 24);
        EXPECT_EQ(model->getSurface(0).getNumTriangles(), 12);
    }
}

TEST_F(ModelTest, LoadFbxModel)
{
    auto inputPath = _context.getTestResourcePath() + "fbx/test_cube.fbx";

    auto importer = GlobalModelFormatManager().getImporter("FBX");
    EXPECT_TRUE(importer) << "No FBX importer available";

    auto model = importer->loadModelFromPath(inputPath);

    EXPECT_TRUE(model) << "No FBX model has been loaded";
    EXPECT_EQ(model->getSurfaceCount(), 1);
    EXPECT_EQ(model->getSurface(0).getDefaultMaterial(), "phong1");
    EXPECT_EQ(model->getVertexCount(), 24);
    EXPECT_EQ(model->getPolyCount(), 12);
}

// #5964: Model nodes below a func_emitter didn't get rendered at the entity's origin after creating the entity
TEST_F(ModelTest, NullModelTransformAfterSceneInsertion)
{
    auto entityOrigin = Vector3(320, 100, 0);
    auto funcEmitter = GlobalEntityModule().createEntityFromSelection("func_emitter", entityOrigin);

    EXPECT_EQ(funcEmitter->getEntity().getKeyValue("model"), "-") << "Expected func_emitter to have a model after creation";

    scene::addNodeToContainer(funcEmitter, GlobalMapModule().getRoot());

    // Get the child model node of the func_emitter
    auto nullModelNode = algorithm::getNthChild(funcEmitter, 0);

    EXPECT_TRUE(nullModelNode) << "func_emitter should have a child node";
    EXPECT_EQ(nullModelNode->getNodeType(), scene::INode::Type::Model) << "func_emitter should have a model node as child";

    // Check the localToWorld() matrix of the model node, it should be a translation to the entity's origin
    auto modelTranslation = nullModelNode->localToWorld().tCol().getVector3();
    EXPECT_TRUE(math::isNear(modelTranslation, entityOrigin, 0.01)) << 
        "Model node should be at the entity's origin, but was at " << modelTranslation;
}

}
