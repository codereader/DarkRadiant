#include "RadiantTest.h"

#include "imodel.h"
#include "imodelcache.h"
#include "imap.h"
#include "ieclass.h"
#include "ientity.h"
#include "algorithm/Scene.h"
#include "scenelib.h"
#include "os/path.h"
#include "string/case_conv.h"

namespace test
{

using ModelExportTest = RadiantTest;

TEST_F(ModelExportTest, ExportPatchMesh)
{
    // Load a map with a cylinder patch
    loadMap("modelexport_patch.map");

    auto patchNode = algorithm::findFirstPatchWithMaterial(GlobalMapModule().findOrInsertWorldspawn(), 
        "textures/darkmod/wood/boards/ship_hull_medium");
    EXPECT_TRUE(patchNode);

    Node_setSelected(patchNode, true);

    // Choose a file in our temp data folder
    std::string modRelativePath = "models/temp/temp_patch.lwo";

    fs::path outputFilename = _context.getTestProjectPath();
    outputFilename /= modRelativePath;
    os::makeDirectory(outputFilename.parent_path().string());

    auto exporter = GlobalModelFormatManager().getExporter("lwo");

    cmd::ArgumentList argList;

    argList.push_back(outputFilename.string());
    argList.push_back(std::string("lwo"));
    argList.push_back(true); // centerObjects
    argList.push_back(true); // skipCaulk
    argList.push_back(false); // replaceSelectionWithModel
    argList.push_back(false); // useEntityOrigin
    argList.push_back(false); // exportLightsAsObjects

    GlobalCommandSystem().executeCommand("ExportSelectedAsModel", argList);

    auto model = GlobalModelCache().getModel(modRelativePath);
    
    EXPECT_TRUE(model);

    // We expect exactly the same amount of vertices as in the patch mesh
    // This ensures that no vertices have been duplicated or faces separated
    auto patch = std::dynamic_pointer_cast<IPatchNode>(patchNode);
    EXPECT_EQ(model->getVertexCount(), patch->getPatch().getTesselatedPatchMesh().vertices.size());

    // Clean up the file
    fs::remove(outputFilename);
}

// #5658: Model exporter failed to write the file if the folder doesn't exist
TEST_F(ModelExportTest, ExportFolderNotExisting)
{
    // Load a map with a cylinder patch
    loadMap("modelexport_patch.map");

    auto patchNode = algorithm::findFirstPatchWithMaterial(GlobalMapModule().findOrInsertWorldspawn(),
        "textures/darkmod/wood/boards/ship_hull_medium");
    EXPECT_TRUE(patchNode);

    Node_setSelected(patchNode, true);

    // A temporary location
    fs::path outputFolder = _context.getTemporaryDataPath();
    outputFolder /= "nonexistingfolder";

    EXPECT_FALSE(fs::exists(outputFolder)) << "Output folder shouldn't exist";

    auto exporter = GlobalModelFormatManager().getExporter("lwo");

    constexpr const char* filename = "test.lwo";
    auto outputPath = outputFolder / filename;
    
    EXPECT_NO_THROW(exporter->exportToPath(outputFolder.string(), filename)) << "Exporter failed to export to " << outputPath;

    EXPECT_TRUE(fs::exists(outputPath)) << "Exporter didn't create the file " << outputPath;
}

inline bool surfaceHasVertex(const model::IModelSurface& surface, const std::function<bool(const ArbitraryMeshVertex&)>& functor)
{
    for (int i = 0; i < surface.getNumVertices(); ++i)
    {
        if (functor(surface.getVertex(i)))
        {
            return true;
        }
    }

    return false;
}

const std::string CustomMaterialName = "custom_surface_name";
const Vector3 VertexColour1(0.1, 0.2, 0.3);
const Vector3 VertexColour2(0.4, 0.5, 0.6);
const Vector3 VertexColour3(0.7, 0.8, 0.9);

inline void checkVertexColoursOfExportedModel(const model::IModelExporterPtr& exporter, const std::string& outputPath_)
{
    fs::path outputPath = outputPath_;
    outputPath /= "models/";
    fs::path filename = "dummy.lwo";

    EXPECT_FALSE(fs::exists(outputPath / filename)) << filename << " already exists in " << outputPath.string();

    exporter->exportToPath(outputPath.string(), filename.string());

    EXPECT_TRUE(fs::exists(outputPath / filename)) << filename << " should exists in " << outputPath.string();

    try
    {
        // Create a func_static using this new model
        auto eclass = GlobalEntityClassManager().findClass("func_static");
        auto entity = GlobalEntityModule().createEntity(eclass);

        scene::addNodeToContainer(entity, GlobalMapModule().getRoot());

        // This should assign the model node to the entity
        Node_getEntity(entity)->setKeyValue("model", "models/" + filename.string());

        // Locate the IModel node among the entity's children
        model::ModelNodePtr model;
        entity->foreachNode([&](const scene::INodePtr& node)
        {
            if (Node_isModel(node))
            {
                model = Node_getModel(node);
            }
            return true;
        });

        EXPECT_TRUE(model) << "Could not locate the model node of the entity";

        EXPECT_EQ(model->getIModel().getSurfaceCount(), 1);

        const auto& surface = model->getIModel().getSurface(0);
        EXPECT_EQ(surface.getDefaultMaterial(), CustomMaterialName);

        EXPECT_EQ(surface.getNumVertices(), 3);

        // The three colours we exported need to be present in the mesh
        EXPECT_TRUE(surfaceHasVertex(surface, [&](const ArbitraryMeshVertex& vertex)
        {
            return math::isNear(vertex.colour, VertexColour1, 0.01);
        }));
        EXPECT_TRUE(surfaceHasVertex(surface, [&](const ArbitraryMeshVertex& vertex)
        {
            return math::isNear(vertex.colour, VertexColour2, 0.01);
        }));
        EXPECT_TRUE(surfaceHasVertex(surface, [&](const ArbitraryMeshVertex& vertex)
        {
            return math::isNear(vertex.colour, VertexColour3, 0.01);
        }));

        fs::remove(outputPath / filename);
    }
    catch (const std::exception&)
    {
        fs::remove(outputPath / filename);
        throw;
    }
}

// #5717: LWO Model exporter didn't write any vertex colours
TEST_F(ModelExportTest, LwoVertexColoursAddedByPolygon)
{
    auto exporter = GlobalModelFormatManager().getExporter("lwo");
    EXPECT_TRUE(exporter);

    // Create a few vertices with custom colours
    std::vector<model::ModelPolygon> polys;

    polys.emplace_back(model::ModelPolygon
    {
        ArbitraryMeshVertex(Vertex3f(1,0,0), Normal3f(1,0,0), TexCoord2f(1,0), VertexColour1),
        ArbitraryMeshVertex(Vertex3f(0,1,0), Normal3f(1,0,0), TexCoord2f(0,0), VertexColour2),
        ArbitraryMeshVertex(Vertex3f(1,1,0), Normal3f(1,0,0), TexCoord2f(1,0), VertexColour3)
    });

    exporter->addPolygons(CustomMaterialName, polys, Matrix4::getIdentity());

    checkVertexColoursOfExportedModel(exporter, _context.getTestProjectPath());
}

class TestModelSurface :
    public model::IIndexedModelSurface
{
public:
    std::vector<ArbitraryMeshVertex> vertices;
    std::vector<unsigned int> indices;

    int getNumVertices() const override
    {
        return static_cast<int>(vertices.size());
    }

    int getNumTriangles() const override
    {
        return static_cast<int>(indices.size() / 3);
    }

    const ArbitraryMeshVertex& getVertex(int vertexNum) const override
    {
        return vertices[vertexNum];
    }
    
    model::ModelPolygon getPolygon(int polygonNum) const override
    {
        return model::ModelPolygon
        {
            vertices[polygonNum * 3 + 0],
            vertices[polygonNum * 3 + 1],
            vertices[polygonNum * 3 + 2]
        };
    }
    
    const std::string& getDefaultMaterial() const override
    {
        return CustomMaterialName;
    }
    
    const std::string& getActiveMaterial() const override
    {
        return getDefaultMaterial();
    }
    
    const std::vector<ArbitraryMeshVertex>& getVertexArray() const override
    {
        return vertices;
    }
    
    const std::vector<unsigned int>& getIndexArray() const override
    {
        return indices;
    }
};

TEST_F(ModelExportTest, LwoVertexColoursAddedBySurface)
{
    auto exporter = GlobalModelFormatManager().getExporter("lwo");
    EXPECT_TRUE(exporter);

    // Create a few vertices with custom colours
    TestModelSurface surface;

    surface.vertices.emplace_back(Vertex3f(1, 0, 0), Normal3f(1, 0, 0), TexCoord2f(1, 0), VertexColour1);
    surface.vertices.emplace_back(Vertex3f(0, 1, 0), Normal3f(1, 0, 0), TexCoord2f(0, 0), VertexColour2);
    surface.vertices.emplace_back(Vertex3f(1, 1, 0), Normal3f(1, 0, 0), TexCoord2f(1, 0), VertexColour3);
    surface.indices.push_back(0);
    surface.indices.push_back(1);
    surface.indices.push_back(2);
    
    exporter->addSurface(surface, Matrix4::getIdentity());

    checkVertexColoursOfExportedModel(exporter, _context.getTestProjectPath());
}

inline void runConverterCode(const std::string& inputPath, const std::string& outputPath)
{
    auto extension = string::to_upper_copy(os::getExtension(outputPath));

    EXPECT_FALSE(fs::exists(outputPath)) << outputPath << " already exists";

    // Invoke the converter code
    GlobalCommandSystem().executeCommand("ConvertModel", cmd::ArgumentList{ inputPath, outputPath, extension });

    EXPECT_TRUE(fs::exists(outputPath)) << outputPath << " should have been created";

    auto importer = GlobalModelFormatManager().getImporter(extension);
    auto exportedModel = importer->loadModelFromPath(outputPath);

    EXPECT_TRUE(exportedModel);
    EXPECT_EQ(exportedModel->getSurfaceCount(), 3);
    EXPECT_EQ(exportedModel->getVertexCount(), 180);
    EXPECT_EQ(exportedModel->getPolyCount(), 258);
}

TEST_F(ModelExportTest, ConvertLwoToAse)
{
    auto outputPath = _context.getTemporaryDataPath() + "conversiontest.ase";
    runConverterCode(_context.getTestProjectPath() + "models/torch.lwo", outputPath);
}

TEST_F(ModelExportTest, ConvertFbxToAse)
{
    auto outputPath = _context.getTemporaryDataPath() + "conversiontest.lwo";
    auto extension = string::to_upper_copy(os::getExtension(outputPath));
    auto inputPath = _context.getTestResourcePath() + "fbx/test_cube.fbx";

    EXPECT_FALSE(fs::exists(outputPath)) << outputPath << " already exists";

    // Invoke the converter code
    GlobalCommandSystem().executeCommand("ConvertModel", cmd::ArgumentList{ inputPath, outputPath, extension });

    EXPECT_TRUE(fs::exists(outputPath)) << outputPath << " should have been created";

    auto importer = GlobalModelFormatManager().getImporter(extension);
    auto exportedModel = importer->loadModelFromPath(outputPath);

    EXPECT_TRUE(exportedModel) << "No FBX model has been created";
    EXPECT_EQ(exportedModel->getSurfaceCount(), 1);
    EXPECT_EQ(exportedModel->getSurface(0).getDefaultMaterial(), "phong1");
    EXPECT_EQ(exportedModel->getVertexCount(), 8);
    EXPECT_EQ(exportedModel->getPolyCount(), 12);
}

TEST_F(ModelExportTest, ConvertAseToLwo)
{
    // Convert the torch to ASE first, then back to LWO
    auto aseOutputPath = _context.getTemporaryDataPath() + "conversiontest.ase";
    runConverterCode(_context.getTestProjectPath() + "models/torch.lwo", aseOutputPath);

    auto lwoOutputPath = _context.getTemporaryDataPath() + "conversiontest.lwo";
    runConverterCode(aseOutputPath, lwoOutputPath);
}

}
