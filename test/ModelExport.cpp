#include "RadiantTest.h"

#include "imodel.h"
#include "imodelcache.h"
#include "imap.h"
#include "ieclass.h"
#include "ientity.h"
#include "ModelExportOptions.h"
#include "algorithm/Primitives.h"
#include "algorithm/Scene.h"
#include "scenelib.h"
#include "os/file.h"
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

    // ExportSelectedAsModel <Path> <ExportFormat> [<ExportOrigin>] [<OriginEntityName>] [<CustomOrigin>][<SkipCaulk>][<ReplaceSelectionWithModel>][<ExportLightsAsObjects>]
    argList.push_back(outputFilename.string());
    argList.push_back(std::string("lwo"));
    argList.push_back(model::getExportOriginString(model::ModelExportOrigin::SelectionCenter)); // centerObjects
    argList.push_back(std::string()); // OriginEntityName
    argList.push_back(Vector3()); // CustomOrigin
    argList.push_back(true); // skipCaulk
    argList.push_back(false); // replaceSelectionWithModel
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

inline bool surfaceHasVertex(const model::IModelSurface& surface, const std::function<bool(const MeshVertex&)>& functor)
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
const Vector4 VertexColour1(0.1, 0.2, 0.3, 0.11);
const Vector4 VertexColour2(0.4, 0.5, 0.6, 0.12);
const Vector4 VertexColour3(0.7, 0.8, 0.9, 0.13);

inline model::ModelNodePtr getChildModelNode(const scene::INodePtr& entity)
{
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

    return model;
}

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
        model::ModelNodePtr model = getChildModelNode(entity);

        EXPECT_TRUE(model) << "Could not locate the model node of the entity";

        EXPECT_EQ(model->getIModel().getSurfaceCount(), 1);

        const auto& surface = model->getIModel().getSurface(0);
        EXPECT_EQ(surface.getDefaultMaterial(), CustomMaterialName);

        EXPECT_EQ(surface.getNumVertices(), 3);

        // The three colours we exported need to be present in the mesh
        EXPECT_TRUE(surfaceHasVertex(surface, [&](const MeshVertex& vertex)
        {
            return math::isNear(vertex.colour, VertexColour1, 0.01);
        }));
        EXPECT_TRUE(surfaceHasVertex(surface, [&](const MeshVertex& vertex)
        {
            return math::isNear(vertex.colour, VertexColour2, 0.01);
        }));
        EXPECT_TRUE(surfaceHasVertex(surface, [&](const MeshVertex& vertex)
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
        MeshVertex(Vertex3(1,0,0), Normal3(1,0,0), TexCoord2f(1,0), VertexColour1),
        MeshVertex(Vertex3(0,1,0), Normal3(1,0,0), TexCoord2f(0,0), VertexColour2),
        MeshVertex(Vertex3(1,1,0), Normal3(1,0,0), TexCoord2f(1,0), VertexColour3)
    });

    exporter->addPolygons(CustomMaterialName, polys, Matrix4::getIdentity());

    checkVertexColoursOfExportedModel(exporter, _context.getTestProjectPath());
}

class TestModelSurface :
    public model::IIndexedModelSurface
{
public:
    std::vector<MeshVertex> vertices;
    std::vector<unsigned int> indices;

    int getNumVertices() const override
    {
        return static_cast<int>(vertices.size());
    }

    int getNumTriangles() const override
    {
        return static_cast<int>(indices.size() / 3);
    }

    const MeshVertex& getVertex(int vertexNum) const override
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

    const std::vector<MeshVertex>& getVertexArray() const override
    {
        return vertices;
    }

    const std::vector<unsigned int>& getIndexArray() const override
    {
        return indices;
    }

    const AABB& getSurfaceBounds() const override
    {
        static AABB aabb;
        aabb = AABB();

        for (const auto& vertex : vertices)
        {
            aabb.includePoint(vertex);
        }

        return aabb;
    }
};

TEST_F(ModelExportTest, LwoVertexColoursAddedBySurface)
{
    auto exporter = GlobalModelFormatManager().getExporter("lwo");
    EXPECT_TRUE(exporter);

    // Create a few vertices with custom colours
    TestModelSurface surface;

    surface.vertices.emplace_back(Vertex3(1, 0, 0), Normal3(1, 0, 0), TexCoord2f(1, 0), VertexColour1);
    surface.vertices.emplace_back(Vertex3(0, 1, 0), Normal3(1, 0, 0), TexCoord2f(0, 0), VertexColour2);
    surface.vertices.emplace_back(Vertex3(1, 1, 0), Normal3(1, 0, 0), TexCoord2f(1, 0), VertexColour3);
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
    auto outputPath = _context.getTemporaryDataPath() + "conversiontest.ase";
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
    EXPECT_EQ(exportedModel->getVertexCount(), 24);
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

// #5659: Exporting a model should be reloaded automatically
TEST_F(ModelExportTest, ExportedModelTriggersEntityRefresh)
{
    auto originalModelMaterial = "textures/numbers/1";
    auto changedModelMaterial = "textures/numbers/2";
    auto modRelativePath = "models/refresh_test.ase";
    auto aseOutputPath = _context.getTestProjectPath() + modRelativePath;

    auto brush = algorithm::createCubicBrush(
        GlobalMapModule().findOrInsertWorldspawn(), Vector3(0, 0, 0), originalModelMaterial);

    GlobalSelectionSystem().setSelectedAll(false);
    Node_setSelected(brush, true);

    // Export this model to the mod-relative location
    GlobalCommandSystem().executeCommand("ExportSelectedAsModel", { aseOutputPath, cmd::Argument("ase") });

    // Create two entities referencing this new model
    auto eclass = GlobalEntityClassManager().findClass("func_static");
    auto entity1 = GlobalEntityModule().createEntity(eclass);
    auto entity2 = GlobalEntityModule().createEntity(eclass);

    scene::addNodeToContainer(entity1, GlobalMapModule().getRoot());
    scene::addNodeToContainer(entity2, GlobalMapModule().getRoot());

    // This should assign the model node to the entity
    Node_getEntity(entity1)->setKeyValue("model", modRelativePath);
    Node_getEntity(entity2)->setKeyValue("model", modRelativePath);
    model::ModelNodePtr model1 = getChildModelNode(entity1);
    model::ModelNodePtr model2 = getChildModelNode(entity2);

    EXPECT_TRUE(model1) << "Could not locate the model node of the entity";
    EXPECT_TRUE(model2) << "Could not locate the model node of the entity";
    EXPECT_EQ(model1->getIModel().getSurface(0).getDefaultMaterial(), originalModelMaterial);
    EXPECT_EQ(model2->getIModel().getSurface(0).getDefaultMaterial(), originalModelMaterial);

    // Now change the brush texture and re-export the model
    Node_getIBrush(brush)->setShader(changedModelMaterial);

    GlobalSelectionSystem().setSelectedAll(false);
    Node_setSelected(brush, true);

    // Re-export this model to the mod-relative location
    GlobalCommandSystem().executeCommand("ExportSelectedAsModel", { aseOutputPath, cmd::Argument("ase") });

    // If all went well, the entity has automatically refreshed its model
    model1 = getChildModelNode(entity1);
    model2 = getChildModelNode(entity2);

    EXPECT_TRUE(model1) << "Could not locate the model node of the entity";
    EXPECT_TRUE(model2) << "Could not locate the model node of the entity";
    EXPECT_EQ(model1->getIModel().getSurface(0).getDefaultMaterial(), changedModelMaterial);
    EXPECT_EQ(model2->getIModel().getSurface(0).getDefaultMaterial(), changedModelMaterial);

    fs::remove(aseOutputPath);
}

// #5705: "Replace Selection with exported Model" should preserve spawnargs
// #5858: "Replace Selection with exported Model" sets classname to "func_static".
TEST_F(ModelExportTest, ExportedModelInheritsSpawnargs)
{
    auto modelPath = "models/torch.lwo";
    auto exportedModelPath = "models/export_test.lwo";
    auto fullModelPath = _context.getTestProjectPath() + exportedModelPath;

    // Create an entity referencing this new model, let's use a mover door
    auto eclass = GlobalEntityClassManager().findClass("atdm:mover_door");
    auto entity = GlobalEntityModule().createEntity(eclass);

    scene::addNodeToContainer(entity, GlobalMapModule().getRoot());

    // This should assign the model node to the entity
    Node_getEntity(entity)->setKeyValue("model", modelPath);

    auto brush = algorithm::createCubicBrush(
        GlobalMapModule().findOrInsertWorldspawn(), Vector3(0, 0, 0), "textures/numbers/1");

    // Add another entity, which is just around to be selected
    auto entity2 = GlobalEntityModule().createEntity(eclass);
    scene::addNodeToContainer(entity2, GlobalMapModule().getRoot());
    Node_getEntity(entity2)->setKeyValue("name", "HenryTheFifth");
    Node_getEntity(entity2)->setKeyValue("henrys_key", "henrys_value");

    // Set some spawnargs which should be preserved after export and give it a name
    Node_getEntity(entity)->setKeyValue("name", "HarryTheTorch");
    Node_getEntity(entity)->setKeyValue("dummy1", "value1");
    Node_getEntity(entity)->setKeyValue("dummy2", "value2");

    // Select the named entity as last item, it should dictate which spawnargs to preserve
    GlobalSelectionSystem().setSelectedAll(false);
    Node_setSelected(brush, true);
    Node_setSelected(entity2, true);
    Node_setSelected(entity, true);

    // Export this model to a mod-relative location

    cmd::ArgumentList argList;

    // ExportSelectedAsModel <Path> <ExportFormat> [<ExportOrigin>] [<OriginEntityName>] [<CustomOrigin>][<SkipCaulk>][<ReplaceSelectionWithModel>][<ExportLightsAsObjects>]
    argList.push_back(fullModelPath);
    argList.push_back(os::getExtension(exportedModelPath)); // lwo
    argList.push_back(model::getExportOriginString(model::ModelExportOrigin::MapOrigin)); // don't center objects
    argList.push_back(std::string()); // OriginEntityName
    argList.push_back(Vector3()); // CustomOrigin
    argList.push_back(false); // skipCaulk
    argList.push_back(true); // replaceSelectionWithModel
    argList.push_back(false); // exportLightsAsObjects

    GlobalCommandSystem().executeCommand("ExportSelectedAsModel", argList);

    // The entities and the brush should have been replaced (remove from the scene, no parent)
    EXPECT_FALSE(entity->getParent());
    EXPECT_FALSE(entity2->getParent());
    EXPECT_FALSE(brush->getParent());

    // Henry the fifth should be gone
    auto henry = algorithm::getEntityByName(GlobalMapModule().getRoot(), "HenryTheFifth");
    EXPECT_FALSE(henry);

    auto newEntityNode = algorithm::getEntityByName(GlobalMapModule().getRoot(), "HarryTheTorch");

    EXPECT_TRUE(newEntityNode) << "Could not locate the named entity after replacing it with a model";
    auto newEntity = Node_getEntity(newEntityNode);

    EXPECT_EQ(newEntity->getKeyValue("name"), "HarryTheTorch");
    EXPECT_EQ(newEntity->getKeyValue("model"), exportedModelPath);
    EXPECT_EQ(newEntity->getKeyValue("dummy1"), "value1");
    EXPECT_EQ(newEntity->getKeyValue("dummy2"), "value2");
    EXPECT_EQ(newEntity->getKeyValue("classname"), "atdm:mover_door");
    EXPECT_EQ(newEntity->getEntityClass()->getDeclName(), "atdm:mover_door");

    // This one should not have been inherited, it was belonging to the other entity
    EXPECT_EQ(newEntity->getKeyValue("henrys_key"), "");

    fs::remove(fullModelPath);
}

// #55687: "Replace Selection with exported Model" should accumulate layers of selection
TEST_F(ModelExportTest, ExportedModelInheritsLayers)
{
    auto modelPath = "models/torch.lwo";
    auto exportedModelPath = "models/export_test.lwo";
    auto fullModelPath = _context.getTestProjectPath() + exportedModelPath;

    // Create an entity referencing this new model
    auto eclass = GlobalEntityClassManager().findClass("func_static");
    auto entity = GlobalEntityModule().createEntity(eclass);

    scene::addNodeToContainer(entity, GlobalMapModule().getRoot());

    // This should assign the model node to the entity
    Node_getEntity(entity)->setKeyValue("model", modelPath);

    auto brush1 = algorithm::createCubicBrush(
        GlobalMapModule().findOrInsertWorldspawn(), Vector3(0, 0, 0), "textures/numbers/1");
    auto brush2 = algorithm::createCubicBrush(
        GlobalMapModule().findOrInsertWorldspawn(), Vector3(0, 0, 0), "textures/numbers/2");
    auto brush3 = algorithm::createCubicBrush(
        GlobalMapModule().findOrInsertWorldspawn(), Vector3(0, 0, 0), "textures/numbers/3");

    auto layer1 = GlobalMapModule().getRoot()->getLayerManager().createLayer("Layer1");
    auto layer2 = GlobalMapModule().getRoot()->getLayerManager().createLayer("Layer2");
    auto layer3 = GlobalMapModule().getRoot()->getLayerManager().createLayer("Layer3");

    // Assign the objects to a few layers
    entity->assignToLayers(scene::LayerList{ layer1, layer2 });
    brush1->assignToLayers(scene::LayerList{ layer1 });
    brush2->assignToLayers(scene::LayerList{ layer2 });
    brush3->assignToLayers(scene::LayerList{ layer3 });

    // Select the objects
    GlobalSelectionSystem().setSelectedAll(false);
    Node_setSelected(entity, true);
    Node_setSelected(brush1, true);
    Node_setSelected(brush2, true);
    Node_setSelected(brush3, true);

    // Export this model to a mod-relative location

    cmd::ArgumentList argList;

    // ExportSelectedAsModel <Path> <ExportFormat> [<ExportOrigin>] [<OriginEntityName>] [<CustomOrigin>][<SkipCaulk>][<ReplaceSelectionWithModel>][<ExportLightsAsObjects>]
    argList.push_back(fullModelPath);
    argList.push_back(os::getExtension(exportedModelPath)); // lwo
    argList.push_back(model::getExportOriginString(model::ModelExportOrigin::MapOrigin)); // don't center objects
    argList.push_back(std::string()); // OriginEntityName
    argList.push_back(Vector3()); // CustomOrigin
    argList.push_back(false); // skipCaulk
    argList.push_back(true); // replaceSelectionWithModel
    argList.push_back(false); // exportLightsAsObjects

    GlobalCommandSystem().executeCommand("ExportSelectedAsModel", argList);

    // The entities and the brush should have been replaced (remove from the scene, no parent)
    EXPECT_FALSE(entity->getParent()) << "Node should have been removed from the scene";
    EXPECT_FALSE(brush1->getParent()) << "Node should have been removed from the scene";
    EXPECT_FALSE(brush2->getParent()) << "Node should have been removed from the scene";
    EXPECT_FALSE(brush3->getParent()) << "Node should have been removed from the scene";

    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 1) << "One item should be selected after export";
    auto newEntity = GlobalSelectionSystem().ultimateSelected();

    EXPECT_EQ(Node_getEntity(newEntity)->getKeyValue("model"), exportedModelPath);

    // Check the layers of this new entity, it should contain the union of all layers
    auto layers = newEntity->getLayers();
    EXPECT_EQ(layers.size(), 3) << "New model should be part of 3 layers";
    EXPECT_EQ(layers.count(layer1), 1) << "New model should be part of Layer 1";
    EXPECT_EQ(layers.count(layer2), 1) << "New model should be part of Layer 2";
    EXPECT_EQ(layers.count(layer3), 1) << "New model should be part of Layer 3";

    fs::remove(fullModelPath);
}

TEST_F(ModelExportTest, ExportUsingEntityOrigin)
{
    auto modelPath = "models/torch.lwo";
    auto exportedModelPath = "models/export_test.lwo";
    auto fullModelPath = _context.getTestProjectPath() + exportedModelPath;

    // Create an entity referencing this new model
    auto eclass = GlobalEntityClassManager().findClass("func_static");
    auto entity = GlobalEntityModule().createEntity(eclass);

    scene::addNodeToContainer(entity, GlobalMapModule().getRoot());

    // This should assign the model node to the entity
    Node_getEntity(entity)->setKeyValue("model", modelPath);
    // Offset this entity from the origin
    Node_getEntity(entity)->setKeyValue("origin", "300 400 50");

    Node_setSelected(entity, true);

    // Choose a file in our temp data folder
    std::string modRelativePath = "models/temp/temp_model_keeping_origin.lwo";

    fs::path outputFilename = _context.getTestProjectPath();
    outputFilename /= modRelativePath;
    os::makeDirectory(outputFilename.parent_path().string());

    auto exporter = GlobalModelFormatManager().getExporter("lwo");

    cmd::ArgumentList argList;

    // ExportSelectedAsModel <Path> <ExportFormat> [<ExportOrigin>] [<OriginEntityName>] [<CustomOrigin>][<SkipCaulk>][<ReplaceSelectionWithModel>][<ExportLightsAsObjects>]
    argList.push_back(outputFilename.string());
    argList.push_back(std::string("lwo"));
    argList.push_back(model::getExportOriginString(model::ModelExportOrigin::EntityOrigin)); // center objects
    argList.push_back(Node_getEntity(entity)->getKeyValue("name")); // OriginEntityName
    argList.push_back(Vector3()); // CustomOrigin
    argList.push_back(true); // skipCaulk
    argList.push_back(false); // replaceSelectionWithModel
    argList.push_back(false); // exportLightsAsObjects

    GlobalCommandSystem().executeCommand("ExportSelectedAsModel", argList);

    auto model = GlobalModelCache().getModel(modRelativePath);
    EXPECT_TRUE(model);

    // Model should be centered around the entity's origin
    auto entityOrigin = string::convert<Vector3>(entity->getEntity().getKeyValue("origin"));
    EXPECT_TRUE(math::isNear(model->localAABB().getOrigin(), entity->worldAABB().getOrigin() - entityOrigin, 0.01));
    EXPECT_TRUE(math::isNear(model->localAABB().getExtents(), entity->worldAABB().getExtents(), 0.01));

    // Clean up the file
    fs::remove(outputFilename);
}

TEST_F(ModelExportTest, ExportUsingCustomOrigin)
{
    auto modelPath = "models/torch.lwo";
    auto exportedModelPath = "models/export_test.lwo";
    auto fullModelPath = _context.getTestProjectPath() + exportedModelPath;

    // Create an entity referencing this new model
    auto eclass = GlobalEntityClassManager().findClass("func_static");
    auto entity = GlobalEntityModule().createEntity(eclass);
    scene::addNodeToContainer(entity, GlobalMapModule().getRoot());

    // This should assign the model node to the entity
    Node_getEntity(entity)->setKeyValue("model", modelPath);
    // Offset this entity from the origin
    Node_getEntity(entity)->setKeyValue("origin", "300 400 50");

    Node_setSelected(entity, true);

    // Choose a file in our temp data folder
    std::string modRelativePath = "models/temp/temp_model_keeping_origin.lwo";

    fs::path outputFilename = _context.getTestProjectPath();
    outputFilename /= modRelativePath;
    os::makeDirectory(outputFilename.parent_path().string());

    auto exporter = GlobalModelFormatManager().getExporter("lwo");

    Vector3 customOrigin(70, -17, 600);
    cmd::ArgumentList argList;

    // ExportSelectedAsModel <Path> <ExportFormat> [<ExportOrigin>] [<OriginEntityName>] [<CustomOrigin>][<SkipCaulk>][<ReplaceSelectionWithModel>][<ExportLightsAsObjects>]
    argList.push_back(outputFilename.string());
    argList.push_back(std::string("lwo"));
    argList.push_back(model::getExportOriginString(model::ModelExportOrigin::CustomOrigin)); // center objects
    argList.push_back(std::string()); // OriginEntityName
    argList.push_back(customOrigin); // CustomOrigin
    argList.push_back(true); // skipCaulk
    argList.push_back(false); // replaceSelectionWithModel
    argList.push_back(false); // exportLightsAsObjects

    GlobalCommandSystem().executeCommand("ExportSelectedAsModel", argList);

    auto model = GlobalModelCache().getModel(modRelativePath);
    EXPECT_TRUE(model);

    // Model should be centered around the custom origin
    EXPECT_TRUE(math::isNear(model->localAABB().getOrigin(), entity->worldAABB().getOrigin() - customOrigin, 0.01));
    EXPECT_TRUE(math::isNear(model->localAABB().getExtents(), entity->worldAABB().getExtents(), 0.01));

    // Clean up the file
    fs::remove(outputFilename);
}

// #5997: ExportSelectedAsCollisionModel should auto-create any necessary folders
TEST_F(ModelExportTest, ExportSelectedAsCollisionModelCreatesFolder)
{
    // Create a func_static
    auto eclass = GlobalEntityClassManager().findClass("func_static");
    auto entity = GlobalEntityModule().createEntity(eclass);
    scene::addNodeToContainer(entity, GlobalMapModule().getRoot());

    auto brush = algorithm::createCubicBrush(entity, Vector3(50, 50, 50));

    // Select just the entity
    GlobalSelectionSystem().setSelectedAll(false);
    Node_setSelected(entity, true);

    auto relativeModelPath = "models/some_folder_not_existing_outside_pk4s/testcube.ase";
    auto expectedPhysicalPath = _context.getTestProjectPath() + relativeModelPath;
    expectedPhysicalPath = os::replaceExtension(expectedPhysicalPath, "cm");

    // Make sure the folder is not there
    fs::remove_all(os::getDirectory(expectedPhysicalPath));
    EXPECT_FALSE(os::fileOrDirExists(os::getDirectory(expectedPhysicalPath))) << "Folder should not exist yet";
    EXPECT_FALSE(os::fileOrDirExists(expectedPhysicalPath)) << "Export CM should not exist yet";

    // Export the selection as collision mesh for the torch model
    GlobalCommandSystem().executeCommand("ExportSelectedAsCollisionModel", { relativeModelPath });

    // Both file and folder need to exist, just check the file
    EXPECT_TRUE(os::fileOrDirExists(expectedPhysicalPath)) << "Folder should have been created by ExportSelectedAsCollisionModel";

    // Remove the folder after the test is done
    fs::remove_all(os::getDirectory(expectedPhysicalPath));
}

}
