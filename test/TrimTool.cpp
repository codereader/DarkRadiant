#include "RadiantTest.h"

#include "imap.h"
#include "ibrush.h"
#include "icommandsystem.h"
#include "iselection.h"

#include "render/View.h"
#include "selection/SelectionVolume.h"

#include "algorithm/Primitives.h"
#include "algorithm/Scene.h"
#include "algorithm/View.h"
#include "testutil/CommandFailureHelper.h"

namespace test
{

using TrimToolTest = RadiantTest;

namespace
{

// Selects the top face of the given brush by constructing a camera view looking down
void selectTopFace(const scene::INodePtr& brushNode)
{
	render::View view(true);
	algorithm::constructCameraView(view, brushNode->worldAABB(), Vector3(0, 0, -1), Vector3(-90, 0, 0));

	auto rectangle = selection::Rectangle::ConstructFromPoint(
		Vector2(0, 0), Vector2(8.0 / algorithm::DeviceWidth, 8.0 / algorithm::DeviceHeight));
	ConstructSelectionTest(view, rectangle);

	SelectionVolume test(view);
	GlobalSelectionSystem().selectPoint(test, selection::SelectionSystem::eToggle, true);
}

// Selects a vertical face of the given brush
void selectSideFace(const scene::INodePtr& brushNode)
{
	render::View view(true);
	algorithm::constructCameraView(view, brushNode->worldAABB(), Vector3(-1, 0, 0), Vector3(0, 180, 0));

	auto rectangle = selection::Rectangle::ConstructFromPoint(
		Vector2(0, 0), Vector2(8.0 / algorithm::DeviceWidth, 8.0 / algorithm::DeviceHeight));
	ConstructSelectionTest(view, rectangle);

	SelectionVolume test(view);
	GlobalSelectionSystem().selectPoint(test, selection::SelectionSystem::eToggle, true);
}

// Counts the brush children of the given parent node
std::size_t countBrushes(const scene::INodePtr& parent)
{
	return algorithm::getChildCount(parent, [](const scene::INodePtr& node)
	{
		return Node_isBrush(node);
	});
}

}

TEST_F(TrimToolTest, CreateTrimRequiresArguments)
{
	auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
	auto brush = algorithm::createCubicBrush(worldspawn, { 0, 0, 0 });
	selectTopFace(brush);
	ASSERT_EQ(GlobalSelectionSystem().getSelectedFaceCount(), 1);

	auto brushCountBefore = countBrushes(worldspawn);

	GlobalCommandSystem().executeCommand("CreateTrimForFaces", cmd::ArgumentList{});

	EXPECT_EQ(countBrushes(worldspawn), brushCountBefore) << "No trim brush should be created with missing arguments";
}

TEST_F(TrimToolTest, CreateTrimRequiresFaceSelection)
{
	auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
	algorithm::createCubicBrush(worldspawn, { 0, 0, 0 });

	CommandFailureHelper helper;

	GlobalCommandSystem().executeCommand("CreateTrimForFaces", { 8.0, 4.0, 0, 0 });

	EXPECT_TRUE(helper.executionHasNotBeenPossible()) << "Should fail when no faces are selected";
}

TEST_F(TrimToolTest, CreateTrimRejectsNonPositiveHeight)
{
	auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
	auto brush = algorithm::createCubicBrush(worldspawn, { 0, 0, 0 });
	selectTopFace(brush);
	ASSERT_EQ(GlobalSelectionSystem().getSelectedFaceCount(), 1);

	CommandFailureHelper helper;

	GlobalCommandSystem().executeCommand("CreateTrimForFaces", { 0.0, 4.0, 0, 0 });

	EXPECT_TRUE(helper.messageReceived()) << "Should fail with non-positive height";
}

TEST_F(TrimToolTest, CreateTrimRejectsNonPositiveDepth)
{
	auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
	auto brush = algorithm::createCubicBrush(worldspawn, { 0, 0, 0 });
	selectTopFace(brush);
	ASSERT_EQ(GlobalSelectionSystem().getSelectedFaceCount(), 1);

	CommandFailureHelper helper;

	GlobalCommandSystem().executeCommand("CreateTrimForFaces", { 8.0, -1.0, 0, 0 });

	EXPECT_TRUE(helper.messageReceived()) << "Should fail with non-positive depth";
}

TEST_F(TrimToolTest, CreateTrimBottomFit)
{
	auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
	auto brush = algorithm::createCubicBrush(worldspawn, { 0, 0, 0 });

	auto brushCountBefore = countBrushes(worldspawn);

	selectTopFace(brush);
	ASSERT_EQ(GlobalSelectionSystem().getSelectedFaceCount(), 1);

	// fitTo=0 (bottom), mitered=0
	GlobalCommandSystem().executeCommand("CreateTrimForFaces", { 16.0, 4.0, 0, 0 });

	EXPECT_EQ(countBrushes(worldspawn), brushCountBefore + 1) << "One trim brush should have been created";

	// The face should have been deselected
	EXPECT_EQ(GlobalSelectionSystem().getSelectedFaceCount(), 0);

	// The new trim brush should be selected
	EXPECT_EQ(GlobalSelectionSystem().countSelected(), 1);

	// Verify the trim brush has 6 faces (a valid cuboid)
	auto selectedNode = GlobalSelectionSystem().ultimateSelected();
	auto* trimBrush = Node_getIBrush(selectedNode);
	ASSERT_NE(trimBrush, nullptr);
	EXPECT_EQ(trimBrush->getNumFaces(), 6);
}

TEST_F(TrimToolTest, CreateTrimTopFit)
{
	auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
	auto brush = algorithm::createCubicBrush(worldspawn, { 0, 0, 0 });

	auto brushCountBefore = countBrushes(worldspawn);

	selectTopFace(brush);
	ASSERT_EQ(GlobalSelectionSystem().getSelectedFaceCount(), 1);

	// fitTo=1 (top), mitered=0
	GlobalCommandSystem().executeCommand("CreateTrimForFaces", { 16.0, 4.0, 1, 0 });

	EXPECT_EQ(countBrushes(worldspawn), brushCountBefore + 1);

	auto selectedNode = GlobalSelectionSystem().ultimateSelected();
	auto* trimBrush = Node_getIBrush(selectedNode);
	ASSERT_NE(trimBrush, nullptr);
	EXPECT_EQ(trimBrush->getNumFaces(), 6);
}

TEST_F(TrimToolTest, CreateTrimLeftFit)
{
	auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
	auto brush = algorithm::createCubicBrush(worldspawn, { 0, 0, 0 });

	auto brushCountBefore = countBrushes(worldspawn);

	selectTopFace(brush);
	ASSERT_EQ(GlobalSelectionSystem().getSelectedFaceCount(), 1);

	// fitTo=2 (left), mitered=0
	GlobalCommandSystem().executeCommand("CreateTrimForFaces", { 16.0, 4.0, 2, 0 });

	EXPECT_EQ(countBrushes(worldspawn), brushCountBefore + 1);

	auto selectedNode = GlobalSelectionSystem().ultimateSelected();
	auto* trimBrush = Node_getIBrush(selectedNode);
	ASSERT_NE(trimBrush, nullptr);
	EXPECT_EQ(trimBrush->getNumFaces(), 6);
}

TEST_F(TrimToolTest, CreateTrimRightFit)
{
	auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
	auto brush = algorithm::createCubicBrush(worldspawn, { 0, 0, 0 });

	auto brushCountBefore = countBrushes(worldspawn);

	selectTopFace(brush);
	ASSERT_EQ(GlobalSelectionSystem().getSelectedFaceCount(), 1);

	// fitTo=3 (right), mitered=0
	GlobalCommandSystem().executeCommand("CreateTrimForFaces", { 16.0, 4.0, 3, 0 });

	EXPECT_EQ(countBrushes(worldspawn), brushCountBefore + 1);

	auto selectedNode = GlobalSelectionSystem().ultimateSelected();
	auto* trimBrush = Node_getIBrush(selectedNode);
	ASSERT_NE(trimBrush, nullptr);
	EXPECT_EQ(trimBrush->getNumFaces(), 6);
}

TEST_F(TrimToolTest, CreateTrimMiteredBottom)
{
	auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
	auto brush = algorithm::createCubicBrush(worldspawn, { 0, 0, 0 });

	auto brushCountBefore = countBrushes(worldspawn);

	selectTopFace(brush);
	ASSERT_EQ(GlobalSelectionSystem().getSelectedFaceCount(), 1);

	// fitTo=0 (bottom), mitered=1
	GlobalCommandSystem().executeCommand("CreateTrimForFaces", { 16.0, 4.0, 0, 1 });

	EXPECT_EQ(countBrushes(worldspawn), brushCountBefore + 1);

	auto selectedNode = GlobalSelectionSystem().ultimateSelected();
	auto* trimBrush = Node_getIBrush(selectedNode);
	ASSERT_NE(trimBrush, nullptr);
	EXPECT_EQ(trimBrush->getNumFaces(), 6);
}

TEST_F(TrimToolTest, CreateTrimMiteredLeft)
{
	auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
	auto brush = algorithm::createCubicBrush(worldspawn, { 0, 0, 0 });

	auto brushCountBefore = countBrushes(worldspawn);

	selectTopFace(brush);
	ASSERT_EQ(GlobalSelectionSystem().getSelectedFaceCount(), 1);

	// fitTo=2 (left), mitered=1
	GlobalCommandSystem().executeCommand("CreateTrimForFaces", { 16.0, 4.0, 2, 1 });

	EXPECT_EQ(countBrushes(worldspawn), brushCountBefore + 1);

	auto selectedNode = GlobalSelectionSystem().ultimateSelected();
	auto* trimBrush = Node_getIBrush(selectedNode);
	ASSERT_NE(trimBrush, nullptr);
	EXPECT_EQ(trimBrush->getNumFaces(), 6);
}

TEST_F(TrimToolTest, CreateTrimOnVerticalFace)
{
	auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
	auto brush = algorithm::createCubicBrush(worldspawn, { 0, 0, 0 });

	auto brushCountBefore = countBrushes(worldspawn);

	selectSideFace(brush);
	ASSERT_EQ(GlobalSelectionSystem().getSelectedFaceCount(), 1);

	// fitTo=0 (bottom), mitered=0
	GlobalCommandSystem().executeCommand("CreateTrimForFaces", { 16.0, 4.0, 0, 0 });

	EXPECT_EQ(countBrushes(worldspawn), brushCountBefore + 1);

	auto selectedNode = GlobalSelectionSystem().ultimateSelected();
	auto* trimBrush = Node_getIBrush(selectedNode);
	ASSERT_NE(trimBrush, nullptr);
	EXPECT_EQ(trimBrush->getNumFaces(), 6);
}

TEST_F(TrimToolTest, CreateTrimIsAddedToWorldspawn)
{
	auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
	auto brush = algorithm::createCubicBrush(worldspawn, { 0, 0, 0 });

	selectTopFace(brush);
	ASSERT_EQ(GlobalSelectionSystem().getSelectedFaceCount(), 1);

	GlobalCommandSystem().executeCommand("CreateTrimForFaces", { 16.0, 4.0, 0, 0 });

	// The newly created trim brush should be a child of worldspawn
	auto selectedNode = GlobalSelectionSystem().ultimateSelected();
	ASSERT_NE(selectedNode, nullptr);
	EXPECT_EQ(selectedNode->getParent(), worldspawn);
}

TEST_F(TrimToolTest, CreateTrimInheritsShaderFromFace)
{
	auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
	std::string material = "textures/common/caulk";
	auto brush = algorithm::createCubicBrush(worldspawn, { 0, 0, 0 }, material);

	selectTopFace(brush);
	ASSERT_EQ(GlobalSelectionSystem().getSelectedFaceCount(), 1);

	GlobalCommandSystem().executeCommand("CreateTrimForFaces", { 16.0, 4.0, 0, 0 });

	auto selectedNode = GlobalSelectionSystem().ultimateSelected();
	auto* trimBrush = Node_getIBrush(selectedNode);
	ASSERT_NE(trimBrush, nullptr);

	// All faces of the trim brush should use the same shader as the source face
	for (int i = 0; i < trimBrush->getNumFaces(); ++i)
	{
		EXPECT_EQ(trimBrush->getFace(i).getShader(), material)
			<< "Trim face " << i << " should inherit the source face shader";
	}
}

}
