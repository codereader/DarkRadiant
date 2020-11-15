#include "RadiantTest.h"

#include "ibrush.h"
#include "imap.h"
#include "iselection.h"
#include "scenelib.h"

namespace test
{

using FaceTest = RadiantTest;

inline bool isSane(const Matrix4& matrix)
{
    for (std::size_t i = 0; i < 15; ++i)
    {
        if (std::isnan(matrix[i]) || std::isinf(matrix[i]))
        {
            return false;
        }
    }

    return true;
}

TEST_F(FaceTest, FitTextureWithZeroScale)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();

    auto brushNode = GlobalBrushCreator().createBrush();
    worldspawn->addChildNode(brushNode);

    GlobalSelectionSystem().setSelectedAll(false);
    Node_setSelected(brushNode, true);

    const double size = 15;
    Vector3 startPos(-size, -size, -size);
    Vector3 endPos(size, size, size);
    GlobalCommandSystem().executeCommand("ResizeSelectedBrushesToBounds",
        startPos, endPos, std::string("shader"));

    GlobalSelectionSystem().setSelectedAll(false);

    auto brush = std::dynamic_pointer_cast<IBrushNode>(brushNode);
    brush->getIBrush().evaluateBRep();

    // Apply a texdef with a 0 scale component to the first face
    ShiftScaleRotation scr;

    scr.shift[0] = 0.0f;
    scr.shift[1] = 0.0f;
    scr.scale[0] = 1.0f;
    scr.scale[1] = 0.0f; // zero scale
    scr.rotate = 0.0f;

    auto& face = brush->getIBrush().getFace(0);

    face.setShiftScaleRotation(scr);

    auto matrix = face.getProjectionMatrix();
    EXPECT_FALSE(isSane(matrix)); // 5th matrix component is INF at the least

    // Now fit the texture
    face.fitTexture(1, 1);

    matrix = face.getProjectionMatrix();

    // The whole matrix should be sane now
    EXPECT_TRUE(isSane(matrix)) << "Texture Projection Matrix is not sane after fitting";

    scene::removeNodeFromParent(brushNode);
}

}
