#include "RadiantTest.h"

#include "imap.h"
#include "ipatch.h"
#include "igrid.h"
#include "iselection.h"
#include "scenelib.h"
#include "algorithm/Primitives.h"
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
    GlobalSelectionSystem().SetMode(selection::SelectionSystem::eComponent);
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
    GlobalSelectionSystem().SetMode(selection::SelectionSystem::eComponent);
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

}
