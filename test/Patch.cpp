#include "RadiantTest.h"

#include "imap.h"
#include "ipatch.h"
#include "igrid.h"
#include "iselection.h"
#include "scenelib.h"
#include "algorithm/Primitives.h"
#include "algorithm/Scene.h"
#include "algorithm/View.h"
#include "render/View.h"

namespace test
{

using PatchTest = RadiantTest;

namespace
{

void selectSinglePatchVertexAt(const Vector3& position)
{
    render::View orthoView(false);

    // Construct an orthoview to test-select the patch vertex
    algorithm::constructCenteredOrthoview(orthoView, position);
    auto centeredTest = algorithm::constructOrthoviewSelectionTest(orthoView);

    auto previousComponentCount = GlobalSelectionSystem().countSelectedComponents();

    GlobalSelectionSystem().selectPoint(centeredTest, selection::SelectionSystem::eToggle, false);

    EXPECT_EQ(GlobalSelectionSystem().countSelectedComponents(), previousComponentCount + 1)
        << "1 additional vertex component should be selected now";
}

}

// Checks that snapping a single selected patch vertex is working
TEST_F(PatchTest, SnapVertexToGrid)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();

    // Use some off-grid bounds to generate the patch
    auto patchNode = algorithm::createPatchFromBounds(worldspawn, AABB({ 0,0,0 }, { 11.5, 13, 15 }));
    Node_setSelected(patchNode, true);

    GlobalGrid().setGridSize(GRID_4);

    // Switch to vertex component mode and select a single patch vertex
    GlobalSelectionSystem().setSelectionMode(selection::SelectionMode::Component);
    GlobalSelectionSystem().SetComponentMode(selection::ComponentSelectionMode::Vertex);

    // Pick a vertex from the patch
    const auto& ctrl = Node_getIPatch(patchNode)->getTransformedCtrlAt(0, 2);

    auto vertexAfterSnapping = ctrl.vertex.getSnapped(GlobalGrid().getGridSize());
    EXPECT_FALSE(math::isNear(ctrl.vertex, vertexAfterSnapping, 0.01)) << "Vertex should be off-grid";

    selectSinglePatchVertexAt(ctrl.vertex);

    // Snap selection to grid
    GlobalCommandSystem().executeCommand("SnapToGrid");

    EXPECT_TRUE(math::isNear(ctrl.vertex, vertexAfterSnapping, 0.01)) << "Vertex should be snapped to grid now";
}

// Checks that snapping a single vertex to grid is undoable
TEST_F(PatchTest, VertexSnappingIsUndoable)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();

    // Use some off-grid bounds to generate the patch
    auto patchNode = algorithm::createPatchFromBounds(worldspawn, AABB({ 0,0,0 }, { 11.5, 13, 15 }));
    Node_setSelected(patchNode, true);

    GlobalGrid().setGridSize(GRID_4);

    // Switch to vertex component mode and select a single patch vertex
    GlobalSelectionSystem().setSelectionMode(selection::SelectionMode::Component);
    GlobalSelectionSystem().SetComponentMode(selection::ComponentSelectionMode::Vertex);

    // Pick a vertex from the patch
    const auto& ctrl = Node_getIPatch(patchNode)->getTransformedCtrlAt(0, 2);

    auto vertexBeforeSnapping = ctrl.vertex;
    auto vertexAfterSnapping = ctrl.vertex.getSnapped(GlobalGrid().getGridSize());
    EXPECT_FALSE(math::isNear(ctrl.vertex, vertexAfterSnapping, 0.01)) << "Vertex should be off-grid";

    selectSinglePatchVertexAt(ctrl.vertex);

    // Snap selection to grid
    GlobalCommandSystem().executeCommand("SnapToGrid");

    EXPECT_TRUE(math::isNear(ctrl.vertex, vertexAfterSnapping, 0.01)) << "Vertex should be snapped to grid now";

    GlobalCommandSystem().executeCommand("Undo");
    EXPECT_TRUE(math::isNear(ctrl.vertex, vertexBeforeSnapping, 0.01)) << "Vertex should be reverted and off-grid again";
}

TEST_F(PatchTest, InvertedEndCapInheritsDef2)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();

    GlobalCommandSystem().execute("PatchEndCap");

    auto endcap = algorithm::findFirstPatch(worldspawn, [](auto&) { return true; });
    Node_setSelected(endcap, true);
    EXPECT_FALSE(Node_getIPatch(endcap)->subdivisionsFixed()) << "End cap should not have fixed tesselation";

    // Execute the create inverted end cap command, check the tesselation
    GlobalCommandSystem().execute("CapSelectedPatches invertedendcap");

    // Get the two end caps
    auto invertedEndCap1 = algorithm::findFirstNode(worldspawn, [&](auto& patch) { return patch != endcap; });
    auto invertedEndCap2 = algorithm::findFirstNode(worldspawn, [&](auto& patch) { return patch != endcap && patch != invertedEndCap1; });
    EXPECT_TRUE(invertedEndCap1) << "Couldn't locate the first end cap";
    EXPECT_TRUE(invertedEndCap2) << "Couldn't locate the second end cap";

    EXPECT_FALSE(Node_getIPatch(invertedEndCap1)->subdivisionsFixed()) << "Inverted end caps should not have fixed tesselation";
    EXPECT_FALSE(Node_getIPatch(invertedEndCap2)->subdivisionsFixed()) << "Inverted end caps should not have fixed tesselation";
}

TEST_F(PatchTest, InvertedEndCapInheritsDef3)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();

    GlobalCommandSystem().execute("PatchEndCap");

    auto endcap = algorithm::findFirstPatch(worldspawn, [](auto&) { return true; });
    Node_setSelected(endcap, true);
    auto subdivisions = Subdivisions(4, 2);
    Node_getIPatch(endcap)->setFixedSubdivisions(true, subdivisions);

    EXPECT_TRUE(Node_getIPatch(endcap)->subdivisionsFixed()) << "End cap should have fixed tesselation now";

    // Execute the create inverted end cap command, check the tesselation
    GlobalCommandSystem().execute("CapSelectedPatches invertedendcap");

    // Get the two end caps
    auto invertedEndCap1 = algorithm::findFirstNode(worldspawn, [&](auto& patch) { return patch != endcap; });
    auto invertedEndCap2 = algorithm::findFirstNode(worldspawn, [&](auto& patch) { return patch != endcap && patch != invertedEndCap1; });
    EXPECT_TRUE(invertedEndCap1) << "Couldn't locate the first end cap";
    EXPECT_TRUE(invertedEndCap2) << "Couldn't locate the second end cap";

    EXPECT_TRUE(Node_getIPatch(invertedEndCap1)->subdivisionsFixed()) << "Inverted end cap 1 should have fixed tesselation now";
    EXPECT_TRUE(Node_getIPatch(invertedEndCap2)->subdivisionsFixed()) << "Inverted end cap 2 should have fixed tesselation now";

    EXPECT_EQ(Node_getIPatch(invertedEndCap1)->getSubdivisions().x(), subdivisions.x()) << "Inverted end cap 1 should have a 4x2 division";
    EXPECT_EQ(Node_getIPatch(invertedEndCap1)->getSubdivisions().y(), subdivisions.y()) << "Inverted end cap 1 should have a 4x2 division";
    EXPECT_EQ(Node_getIPatch(invertedEndCap2)->getSubdivisions().x(), subdivisions.x()) << "Inverted end cap 2 should have a 4x2 division";
    EXPECT_EQ(Node_getIPatch(invertedEndCap2)->getSubdivisions().y(), subdivisions.y()) << "Inverted end cap 2 should have a 4x2 division";
}

inline scene::INodePtr findNodeInLayer(const std::string& layerName)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto layerId = GlobalMapModule().getRoot()->getLayerManager().getLayerID(layerName);

    return algorithm::findFirstNode(worldspawn, [&](const auto& node)
    {
        const auto& layers = node->getLayers();
        return layers.find(layerId) != layers.end();
    });
}

inline IPatch* findPatchInLayerWithMaterial(const std::string& layerName, const std::string& shaderName)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto layerId = GlobalMapModule().getRoot()->getLayerManager().getLayerID(layerName);

    return Node_getIPatch(algorithm::findFirstNode(worldspawn, [&](const auto& node)
    {
        const auto& layers = node->getLayers();
        
        return layers.find(layerId) != layers.end() && Node_getIPatch(node)->getShader() == shaderName;
    }));
}

inline void comparePatches(const IPatch& patch, const IPatch& expected, const std::string& infoText)
{
    EXPECT_EQ(patch.getWidth(), expected.getWidth()) << infoText << ": Width mismatch";
    EXPECT_EQ(patch.getHeight(), expected.getHeight()) << infoText << ": Height mismatch";

    // Check the whole control point matrix
    for (auto row = 0; row < patch.getHeight(); ++row)
    {
        for (auto col = 0; col < patch.getWidth(); ++col)
        {
            const auto& expectedCtrl = expected.ctrlAt(row, col);
            const auto& ctrl = patch.ctrlAt(row, col);

            EXPECT_TRUE(math::isNear(ctrl.vertex, expectedCtrl.vertex, 0.01)) << infoText << ": Vertex mismatch at " << row << "," << col;
        }
    }
}

TEST_F(PatchTest, CapPatch)
{
    loadMap("patch_cap_test.mapx");
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();

    // The cap types to test, there is a layer for each type in the test map named just like this
    auto capTypes = std::vector<std::string>{ "endcap", "cylinder", "bevel", "invertedendcap", "invertedbevel" };

    for (auto capType : capTypes)
    {
        auto layerName = capType;
        auto resultLayerName = fmt::format("{0}_result", capType);

        auto sourcePatch = findNodeInLayer(layerName);

        // Get hold of the result patches that are stored in child layers (the "front" patch is textured with "2")
        auto frontResultPatch = findPatchInLayerWithMaterial(resultLayerName, "textures/numbers/2");
        auto backResultPatch = findPatchInLayerWithMaterial(resultLayerName, "textures/numbers/3");

        // Select the source patch and cap it
        Node_setSelected(sourcePatch, true);
        GlobalCommandSystem().executeCommand("CapSelectedPatches", cmd::ArgumentList{capType});
        Node_setSelected(sourcePatch, false);

        // The "front" patch has the higher Z coordinate
        scene::INodePtr frontNode;
        scene::INodePtr backNode;
        GlobalSelectionSystem().foreachSelected([&](const auto& node)
        {
            if (!frontNode || frontNode->worldAABB().getOrigin().z() < node->worldAABB().getOrigin().z())
            {
                frontNode = node;
            }

            if (!backNode || backNode->worldAABB().getOrigin().z() > node->worldAABB().getOrigin().z())
            {
                backNode = node;
            }
        });

        EXPECT_NE(frontNode, backNode) << "Logic error determining front and back patch";

        auto frontPatch = Node_getIPatch(frontNode);
        auto backPatch = Node_getIPatch(backNode);

        comparePatches(*frontPatch, *frontResultPatch, fmt::format("Front Patch (Cap Type {0})", capType));
        comparePatches(*backPatch, *backResultPatch, fmt::format("Back Patch (Cap Type {0})", capType));

        GlobalSelectionSystem().setSelectedAll(false);
    }
}

}
