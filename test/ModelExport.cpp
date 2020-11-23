#include "RadiantTest.h"

#include "imodel.h"
#include "imodelcache.h"
#include "imap.h"
#include "algorithm/Scene.h"
#include "scenelib.h"
#include "os/path.h"

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

}
