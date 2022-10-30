#include "RadiantTest.h"

#include "iselectable.h"
#include "iselection.h"
#include "ishaderclipboard.h"

#include "testutil/CommandFailureHelper.h"
#include "testutil/MapOperationMonitor.h"
#include "algorithm/Primitives.h"
#include "algorithm/View.h"
#include "algorithm/XmlUtils.h"
#include "render/View.h"

namespace test
{

using ClipboardTest = RadiantTest;

// --- Clipboard related tests ---

TEST_F(ClipboardTest, CopyEmptySelection)
{
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 0) << "Should start with an empty selection";

    // Monitor radiant to catch the messages
    CommandFailureHelper helper;
    MapOperationMonitor operationMonitor;

    // This should do nothing, and it should not throw any execution failures neither
    GlobalCommandSystem().executeCommand("Copy");

    EXPECT_FALSE(helper.messageReceived()) << "Command execution shouldn't have failed";
    EXPECT_TRUE(operationMonitor.messageReceived()) << "Command should have sent out an OperationMessage";
}

TEST_F(ClipboardTest, CutEmptySelection)
{
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 0) << "Should start with an empty selection";

    // Monitor radiant to catch the messages
    CommandFailureHelper helper;
    MapOperationMonitor operationMonitor;

    // This should do nothing, and it should not throw any execution failures neither
    GlobalCommandSystem().executeCommand("Cut");

    EXPECT_FALSE(helper.messageReceived()) << "Command execution shouldn't have failed";
    EXPECT_TRUE(operationMonitor.messageReceived()) << "Command should have sent out an OperationMessage";
}

TEST_F(ClipboardTest, CopyNonEmptySelection)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush = algorithm::createCubicBrush(worldspawn);

    Node_setSelected(brush, true);

    // Monitor radiant to catch the messages
    CommandFailureHelper helper;
    MapOperationMonitor operationMonitor;

    GlobalCommandSystem().executeCommand("Copy");

    EXPECT_FALSE(helper.messageReceived()) << "Command execution should not have failed";
    EXPECT_TRUE(operationMonitor.messageReceived()) << "Command should have sent out an OperationMessage";

    // Check the clipboard contents, it should contain a mapx file
    algorithm::assertStringIsMapxFile(GlobalClipboard().getString());
}

TEST_F(ClipboardTest, CutNonEmptySelection)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush = algorithm::createCubicBrush(worldspawn);

    Node_setSelected(brush, true);

    // Monitor radiant to catch the messages
    CommandFailureHelper helper;
    MapOperationMonitor operationMonitor;

    GlobalCommandSystem().executeCommand("Cut");

    EXPECT_FALSE(helper.messageReceived()) << "Command execution should not have failed";
    EXPECT_TRUE(operationMonitor.messageReceived()) << "Command should have sent out an OperationMessage";

    // Check the clipboard contents, it should contain a mapx file
    algorithm::assertStringIsMapxFile(GlobalClipboard().getString());

    EXPECT_FALSE(Node_isSelected(brush));
    EXPECT_FALSE(brush->getParent()) << "Brush should have been removed from the map";
    EXPECT_FALSE(brush->inScene()) << "Brush should be have been removed from the scene";
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 0) << "Selection should now be empty";
}

TEST_F(ClipboardTest, CutIsUndoable)
{
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush = algorithm::createCubicBrush(worldspawn);
    Node_setSelected(brush, true);

    // Cut and check the clipboard contents, it should contain a mapx file
    GlobalCommandSystem().executeCommand("Cut");
    algorithm::assertStringIsMapxFile(GlobalClipboard().getString());

    EXPECT_FALSE(brush->getParent()) << "Brush should have been removed from the map";
    EXPECT_FALSE(brush->inScene()) << "Brush should be have been removed from the scene";
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 0) << "Selection should now be empty";

    GlobalCommandSystem().executeCommand("Undo");
    EXPECT_TRUE(brush->getParent()) << "Brush should be back now";
    EXPECT_TRUE(brush->inScene()) << "Brush should be back now";
}

TEST_F(ClipboardTest, CopyFaceSelection)
{
    // Create a brush and select a single face
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    auto brush = algorithm::createCubicBrush(worldspawn, { 0, 0 ,0 }, "textures/common/caulk");

    render::View view(true);
    algorithm::constructCameraView(view, brush->worldAABB(), Vector3(0, 0, -1), Vector3(-90, 0, 0));

    auto rectangle = selection::Rectangle::ConstructFromPoint(Vector2(0, 0), Vector2(8.0 / algorithm::DeviceWidth, 8.0 / algorithm::DeviceHeight));
    ConstructSelectionTest(view, rectangle);

    SelectionVolume test(view);
    GlobalSelectionSystem().selectPoint(test, selection::SelectionSystem::eToggle, true);

    EXPECT_EQ(GlobalSelectionSystem().getSelectedFaceCount(), 1) << "One face should be selected now";

    // Monitor radiant to catch the messages
    CommandFailureHelper helper;
    MapOperationMonitor operationMonitor;

    GlobalShaderClipboard().clear();
    EXPECT_NE(GlobalShaderClipboard().getShaderName(), "textures/common/caulk");

    // This should do nothing, and it should not throw any execution failures neither
    GlobalCommandSystem().executeCommand("Copy");

    EXPECT_FALSE(helper.messageReceived()) << "Command execution should not have failed";
    EXPECT_TRUE(operationMonitor.messageReceived()) << "Command should have sent out an OperationMessage";

    // Check the shader clipboard, it should contain the material name
    EXPECT_EQ(GlobalShaderClipboard().getShaderName(), "textures/common/caulk") << "Shaderclipboard should contain the material name now";
}

}
