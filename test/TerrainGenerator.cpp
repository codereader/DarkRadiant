#include "RadiantTest.h"

#include "icommandsystem.h"
#include "imap.h"
#include "ipatch.h"
#include "iundo.h"

#include "algorithm/Scene.h"

namespace test
{

using TerrainGeneratorTest = RadiantTest;

void executeGenerateTerrain(int algorithm, int seed = 42, double frequency = 0.01,
	double amplitude = 64.0, int octaves = 4, double persistence = 0.5,
	double lacunarity = 2.0, double offset = 1.0, int columns = 5, int rows = 5,
	double width = 512.0, double height = 512.0, double spawnX = 0.0,
	double spawnY = 0.0, double spawnZ = 0.0,
	const std::string& material = "textures/common/caulk")
{
	GlobalCommandSystem().executeCommand("GenerateTerrain",
		{ cmd::Argument(algorithm),
		  cmd::Argument(seed),
		  cmd::Argument(frequency),
		  cmd::Argument(amplitude),
		  cmd::Argument(octaves),
		  cmd::Argument(persistence),
		  cmd::Argument(lacunarity),
		  cmd::Argument(offset),
		  cmd::Argument(columns),
		  cmd::Argument(rows),
		  cmd::Argument(width),
		  cmd::Argument(height),
		  cmd::Argument(spawnX),
		  cmd::Argument(spawnY),
		  cmd::Argument(spawnZ),
		  cmd::Argument(material) });
}

TEST_F(TerrainGeneratorTest, BasicTerrainGeneration)
{
	const int columns = 5;
	const int rows = 5;
	const std::string material = "textures/common/caulk";

	executeGenerateTerrain(0, 42, 0.01, 64.0, 4, 0.5, 2.0, 1.0,
		columns, rows, 512.0, 512.0, 0.0, 0.0, 0.0, material);

	auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();

	// Find the generated patch
	auto patchNode = algorithm::findFirstPatchWithMaterial(worldspawn, material);
	ASSERT_TRUE(patchNode) << "Terrain patch should have been created";

	auto* patch = Node_getIPatch(patchNode);
	ASSERT_NE(patch, nullptr);

	// Check that the resulting patch was created
	EXPECT_EQ(patch->getWidth(), columns);
	EXPECT_EQ(patch->getHeight(), rows);
	EXPECT_EQ(patch->getShader(), material);
}

TEST_F(TerrainGeneratorTest, CanUndoTerrainGeneration)
{
	auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
	auto childCountBefore = algorithm::getChildCount(worldspawn);

	// Call terrain generator with default args
	executeGenerateTerrain(0);

	EXPECT_EQ(algorithm::getChildCount(worldspawn), childCountBefore + 1)
		<< "One patch should have been added";

	GlobalUndoSystem().undo();

	EXPECT_EQ(algorithm::getChildCount(worldspawn), childCountBefore)
		<< "Undo should remove the generated terrain patch";
}

} // namespace test
