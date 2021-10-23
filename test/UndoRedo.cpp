#include "RadiantTest.h"

#include "iundo.h"
#include "ibrush.h"
#include "imap.h"
#include "icommandsystem.h"
#include "math/Matrix4.h"
#include "algorithm/Scene.h"
#include "scenelib.h"

namespace test
{

using UndoTest = RadiantTest;

TEST_F(UndoTest, BrushCreation)
{
    std::string mapPath = "maps/simple_brushes.map";
    GlobalCommandSystem().executeCommand("OpenMap", mapPath);

    auto material = "textures/numbers/19";
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    EXPECT_FALSE(algorithm::findFirstBrushWithMaterial(worldspawn, material)) << "Test map already has a brush with that material";

    auto brushNode = GlobalBrushCreator().createBrush();
    auto& brush = *Node_getIBrush(brushNode);

    auto translation = Matrix4::getTranslation({ 20, 3, -7 });
    brush.addFace(Plane3(+1, 0, 0, 64).transform(translation));
    brush.addFace(Plane3(-1, 0, 0, 64).transform(translation));
    brush.addFace(Plane3(0, +1, 0, 64).transform(translation));
    brush.addFace(Plane3(0, -1, 0, 64).transform(translation));
    brush.addFace(Plane3(0, 0, +1, 64).transform(translation));
    brush.addFace(Plane3(0, 0, -1, 64).transform(translation));

    brush.setShader("textures/numbers/19");
    brush.evaluateBRep();
    
    EXPECT_FALSE(GlobalMapModule().isModified()) << "Map already modified before the change";

    {
        UndoableCommand cmd("testOperation");
        scene::addNodeToContainer(brushNode, worldspawn);
    }

    EXPECT_TRUE(GlobalMapModule().isModified()) << "Map should be modified after this change";
    EXPECT_TRUE(algorithm::findFirstBrushWithMaterial(worldspawn, material)) << "Could not locate the brush";

    // Brush should be gone now
    GlobalUndoSystem().undo();
    EXPECT_FALSE(brushNode->inScene());
    EXPECT_FALSE(algorithm::findFirstBrushWithMaterial(worldspawn, material)) << "Brush should be gone after undo";

    // Redo, brush should be back again
    GlobalUndoSystem().redo();
    EXPECT_TRUE(brushNode->inScene());
    EXPECT_TRUE(algorithm::findFirstBrushWithMaterial(worldspawn, material)) << "Could not locate the brush after redo";
}

}
