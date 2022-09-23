#include "RadiantTest.h"

#include "iparticles.h"
#include "iparticlestage.h"
#include "os/path.h"
#include "string/replace.h"
#include "algorithm/FileUtils.h"
#include "parser/DefBlockSyntaxParser.h"
#include "string/case_conv.h"
#include "testutil/TemporaryFile.h"

namespace test
{

class ParticlesTest :
    public RadiantTest
{
public:
    const char* const TEST_PARTICLE_FILE = "testparticles.prt";

    void preStartup() override
    {
        // Remove the physical override_test.prt file
        fs::remove(_context.getTestProjectPath() + "particles/override_test.prt");

        // Create a backup of the testparticles file
        fs::path testFile = _context.getTestProjectPath() + "particles/" + TEST_PARTICLE_FILE;
        fs::path bakFile = testFile;
        bakFile.replace_extension(".bak");
        fs::remove(bakFile);
        fs::copy(testFile, bakFile);
    }

    void preShutdown() override
    {
        fs::path testFile = _context.getTestProjectPath() + "particles/" + TEST_PARTICLE_FILE;
        fs::remove(testFile);
        fs::path bakFile = testFile;
        bakFile.replace_extension(".bak");
        fs::rename(bakFile, testFile);
    }
};

// #5853: Two files define the same particle def:
// 1) in VFS: particles/z_precedence.prt
// 2) in PK4: test_particles.pk4/particles/precedence.prt
// Even though the filesystem folder has higher priority, the file
// in the PK4 should be parsed first, since the lexicographical order
// is considered when looking for .prt files. That's why the
// declaration in the filesystem should not take effect.
TEST_F(ParticlesTest, ParticleFilenamePrecedence)
{
    auto decl = GlobalParticlesManager().getDefByName("precedencecheck");

    EXPECT_TRUE(decl) << "Could not find the particle 'precedencecheck'";

    // The decl in the PK4 should be processed first, the one in the filesystem just produces a warning
    EXPECT_EQ(decl->getStage(0)->getMaterialName(), "textures/common/caulk")
        << "The particle using the caulk texture should have taken precedence";
}

// Prove that the ignored, duplicate particleDef is not stopping the parser from processing the rest of the file
TEST_F(ParticlesTest, ParticleBelowDuplicatedIsParsed)
{
    auto decl = GlobalParticlesManager().getDefByName("particle_after_precedencecheck");

    EXPECT_TRUE(decl) << "Could not locate the particleDef that should go below the precedencecheck in z_precedence.prt";
    EXPECT_EQ(decl->getStage(0)->getMaterialName(), "textures/common/nodraw")
        << "The particle using the caulk texture should have taken precedence";
}

TEST_F(ParticlesTest, FindParticle)
{
    EXPECT_TRUE(GlobalParticlesManager().getDefByName("tdm_fire_torch")) << "tdm_fire_torch not found";
    EXPECT_TRUE(GlobalParticlesManager().getDefByName("firefly_blue")) << "firefly_blue not found";
    EXPECT_TRUE(GlobalParticlesManager().getDefByName("firefly_yellow")) << "mixed case typename particle firefly_yellow not found";
    EXPECT_TRUE(GlobalParticlesManager().getDefByName("firefly_green")) << "particle without typename firefly_green not found";
    EXPECT_TRUE(GlobalParticlesManager().getDefByName("flamejet"));
    EXPECT_TRUE(GlobalParticlesManager().getDefByName("flamejet_in_pk4"));
    EXPECT_TRUE(GlobalParticlesManager().getDefByName("tdm_fire_torch_in_pk4"));

    EXPECT_FALSE(GlobalParticlesManager().getDefByName("flamejet_nonexisting")) << "flamejet_nonexisting doesn't exist";
}

TEST_F(ParticlesTest, ParticleMetadata)
{
    auto flamejet = GlobalParticlesManager().getDefByName("flamejet");

    EXPECT_EQ(flamejet->getDeclName(), "flamejet");
    EXPECT_EQ(flamejet->getDeclType(), decl::Type::Particle);
    EXPECT_EQ(flamejet->getModName(), RadiantTest::DEFAULT_GAME_TYPE);
    EXPECT_EQ(flamejet->getDeclFilePath(), "particles/testparticles.prt");

    auto flamejetInPk4 = GlobalParticlesManager().getDefByName("flamejet_in_pk4");

    EXPECT_EQ(flamejetInPk4->getDeclName(), "flamejet_in_pk4");
    EXPECT_EQ(flamejetInPk4->getDeclType(), decl::Type::Particle);
    EXPECT_EQ(flamejetInPk4->getModName(), RadiantTest::DEFAULT_GAME_TYPE);
    EXPECT_EQ(flamejetInPk4->getDeclFilePath(), "particles/override_test.prt");
}

TEST_F(ParticlesTest, DefProperties)
{
    auto decl = GlobalParticlesManager().getDefByName("flamejet");

    EXPECT_EQ(decl->getDepthHack(), 0.001f);
    EXPECT_EQ(decl->getNumStages(), 3);

    EXPECT_EQ(decl->getStage(0)->getCount(), 20);
    EXPECT_EQ(decl->getStage(0)->getMaterialName(), "textures/particles/pfirebig2");
    EXPECT_EQ(decl->getStage(0)->getDuration(), 0.7f);
    EXPECT_EQ(decl->getStage(0)->getCycles(), 0.0f);
    EXPECT_EQ(decl->getStage(0)->getBunching(), 1.0f);
    EXPECT_EQ(decl->getStage(0)->getDistributionType(), particles::IStageDef::DISTRIBUTION_CYLINDER);
    EXPECT_EQ(decl->getStage(0)->getDistributionParm(0), 4);
    EXPECT_EQ(decl->getStage(0)->getDistributionParm(1), 4);
    EXPECT_EQ(decl->getStage(0)->getDistributionParm(2), 10);
    EXPECT_EQ(decl->getStage(0)->getDistributionParm(3), 0);
    EXPECT_EQ(decl->getStage(0)->getDirectionType(), particles::IStageDef::DIRECTION_CONE);
    EXPECT_EQ(decl->getStage(0)->getDirectionParm(0), 10.0);
    EXPECT_EQ(decl->getStage(0)->getOrientationType(), particles::IStageDef::ORIENTATION_VIEW);
    EXPECT_EQ(decl->getStage(0)->getSpeed().getFrom(), 86);
    EXPECT_EQ(decl->getStage(0)->getSpeed().getTo(), -4);
    EXPECT_EQ(decl->getStage(0)->getSize().getFrom(), 16.5f);
    EXPECT_EQ(decl->getStage(0)->getSize().getTo(), 28.5f);
    EXPECT_EQ(decl->getStage(0)->getAspect().getFrom(), 1.0f);
    EXPECT_EQ(decl->getStage(0)->getAspect().getTo(), 1.0f);
    EXPECT_EQ(decl->getStage(0)->getRotationSpeed().getFrom(), 24.0f);
    EXPECT_EQ(decl->getStage(0)->getRotationSpeed().getTo(), 29.0f);
    EXPECT_EQ(decl->getStage(0)->getFadeInFraction(), 0.35f);
    EXPECT_EQ(decl->getStage(0)->getFadeOutFraction(), 0.2f);
    EXPECT_EQ(decl->getStage(0)->getColour(), Vector4(0.92, 0.92, 0.92, 1));
    EXPECT_EQ(decl->getStage(0)->getFadeColour(), Vector4(0, 0, 0, 1));
    EXPECT_EQ(decl->getStage(0)->getOffset(), Vector3(0, 0, 0));
    EXPECT_EQ(decl->getStage(0)->getGravity(), 10.0f);
    EXPECT_EQ(decl->getStage(0)->getWorldGravityFlag(), false);

    EXPECT_EQ(decl->getStage(1)->getCount(), 4);
    EXPECT_EQ(decl->getStage(1)->getMaterialName(), "textures/particles/dust");
    EXPECT_EQ(decl->getStage(1)->getDuration(), 1.6f);
    EXPECT_EQ(decl->getStage(1)->getCycles(), 0.0f);
    EXPECT_EQ(decl->getStage(1)->getBunching(), 1.0f);
    EXPECT_EQ(decl->getStage(1)->getDistributionType(), particles::IStageDef::DISTRIBUTION_CYLINDER);
    EXPECT_EQ(decl->getStage(1)->getDistributionParm(0), 2);
    EXPECT_EQ(decl->getStage(1)->getDistributionParm(1), 2);
    EXPECT_EQ(decl->getStage(1)->getDistributionParm(2), 2);
    EXPECT_EQ(decl->getStage(1)->getDistributionParm(3), 0);
    EXPECT_EQ(decl->getStage(1)->getDirectionType(), particles::IStageDef::DIRECTION_CONE);
    EXPECT_EQ(decl->getStage(1)->getDirectionParm(0), 90.0);
    EXPECT_EQ(decl->getStage(1)->getOrientationType(), particles::IStageDef::ORIENTATION_VIEW);
    EXPECT_EQ(decl->getStage(1)->getSpeed().getFrom(), 47);
    EXPECT_EQ(decl->getStage(1)->getSpeed().getTo(), 0);
    EXPECT_EQ(decl->getStage(1)->getSize().getFrom(), 13.0f);
    EXPECT_EQ(decl->getStage(1)->getSize().getTo(), 38.5f);
    EXPECT_EQ(decl->getStage(1)->getAspect().getFrom(), 1.5f);
    EXPECT_EQ(decl->getStage(1)->getAspect().getTo(), 1.0f);
    EXPECT_EQ(decl->getStage(1)->getRotationSpeed().getFrom(), 50.0f);
    EXPECT_EQ(decl->getStage(1)->getRotationSpeed().getTo(), 85.0f);
    EXPECT_EQ(decl->getStage(1)->getFadeInFraction(), 1.0f);
    EXPECT_EQ(decl->getStage(1)->getFadeOutFraction(), 0.6f);
    EXPECT_EQ(decl->getStage(1)->getColour(), Vector4(1, 1, 1, 1));
    EXPECT_EQ(decl->getStage(1)->getFadeColour(), Vector4(0, 0, 1, 0));
    EXPECT_EQ(decl->getStage(1)->getOffset(), Vector3(0, 0, 4));
    EXPECT_EQ(decl->getStage(1)->getGravity(), -29.0f);
    EXPECT_EQ(decl->getStage(1)->getWorldGravityFlag(), true);

    // Test a few key properties of stage #3 that are not covered by stages #1 and #2
    EXPECT_EQ(decl->getStage(2)->getDistributionType(), particles::IStageDef::DISTRIBUTION_RECT);
    EXPECT_EQ(decl->getStage(2)->getDistributionParm(0), 3);
    EXPECT_EQ(decl->getStage(2)->getDistributionParm(1), 3);
    EXPECT_EQ(decl->getStage(2)->getDistributionParm(2), 0);
    EXPECT_EQ(decl->getStage(2)->getDirectionType(), particles::IStageDef::DIRECTION_OUTWARD);
    EXPECT_EQ(decl->getStage(2)->getDirectionParm(0), 100.0);
    EXPECT_EQ(decl->getStage(2)->getRotationSpeed().getFrom(), 100.0f);
    EXPECT_EQ(decl->getStage(2)->getRotationSpeed().getTo(), 100.0f);
    EXPECT_EQ(decl->getStage(2)->getOffset(), Vector3(0, 0, -20));
}

TEST_F(ParticlesTest, ForeachParticleDef)
{
    std::set<std::string> visitedDefs;
    GlobalParticlesManager().forEachParticleDef([&](const particles::IParticleDef& def)
    {
        visitedDefs.insert(def.getDeclName());
    });

    EXPECT_EQ(visitedDefs.count("tdm_fire_torch"), 1) << "A particle didn't appear in the list of visited particles";
    EXPECT_EQ(visitedDefs.count("firefly_blue"), 1) << "A particle didn't appear in the list of visited particles";
    EXPECT_EQ(visitedDefs.count("firefly_yellow"), 1) << "A particle didn't appear in the list of visited particles";
    EXPECT_EQ(visitedDefs.count("firefly_green"), 1) << "A particle didn't appear in the list of visited particles";
    EXPECT_EQ(visitedDefs.count("flamejet"), 1) << "A particle didn't appear in the list of visited particles";
    EXPECT_EQ(visitedDefs.count("flamejet_in_pk4"), 1) << "A particle didn't appear in the list of visited particles";
    EXPECT_EQ(visitedDefs.count("tdm_fire_torch_in_pk4"), 1) << "A particle didn't appear in the list of visited particles";

    EXPECT_NE(visitedDefs.count("not_existing"), 1) << "This particle shouldn't appear in the list of visited particles";
}

TEST_F(ParticlesTest, FindOrInsertParticleDef)
{
    EXPECT_TRUE(GlobalParticlesManager().findOrInsertParticleDef("tdm_fire_torch")) << "tdm_fire_torch not found";
    EXPECT_TRUE(GlobalParticlesManager().findOrInsertParticleDef("firefly_blue")) << "firefly_blue not found";
    EXPECT_TRUE(GlobalParticlesManager().findOrInsertParticleDef("firefly_yellow")) << "mixed case typename particle firefly_yellow not found";
    EXPECT_TRUE(GlobalParticlesManager().findOrInsertParticleDef("firefly_green")) << "particle without typename firefly_green not found";
    EXPECT_TRUE(GlobalParticlesManager().findOrInsertParticleDef("flamejet"));

    auto inserted = GlobalParticlesManager().findOrInsertParticleDef("flamejet_nonexisting");
    EXPECT_TRUE(inserted) << "Nonexistent def should have been inserted";

    EXPECT_EQ(inserted->getDeclName(), "flamejet_nonexisting");
    EXPECT_EQ(inserted->getDeclType(), decl::Type::Particle);
    EXPECT_EQ(inserted->getBlockSyntax().contents, "");
}

constexpr const char* ParticleSourceTemplate = R"(

	depthHack	0.001
	{
		count				20
		material			textures/particles/pfirebig2
		time				0.700
		cycles				0.000
		bunching			1.000
		distribution		cylinder 4.000 4.000 10.000 0.000 
		direction			cone 10.000 
		orientation			view 
		speed				 86.000  to "-4.000"
		size				 16.500  to 28.500
		aspect				 1.000 
		rotation				 24.000  to 29.000
		fadeIn				0.350
		fadeOut				0.200
		color 				0.920 0.920 0.920 1.000
		fadeColor 			0.000 0.000 0.000 1.000
		offset 				0.000 0.000 0.000
		gravity 			10.000
	}

)";

inline std::string createParticleSource()
{
    return ParticleSourceTemplate;
}

inline particles::IParticleDef::Ptr createParticleFromSource(const std::string& defName)
{
    auto decl = GlobalParticlesManager().findOrInsertParticleDef(defName);

    auto source = createParticleSource();

    decl::DeclarationBlockSyntax syntax;
    syntax.typeName = "particle";
    syntax.name = defName;
    syntax.contents = source;
    syntax.fileInfo = vfs::FileInfo("particles/", "export_particle_test.prt", vfs::Visibility::NORMAL);
    decl->setBlockSyntax(syntax);
    decl->getDepthHack(); // ensure the particle is parsed
    decl->setFilename(os::getFilename(syntax.fileInfo.fullPath()));

    return decl;
}

// Filename is the leaf name, relative to the particles/ folder
inline void setParticleFilename(const particles::IParticleDef::Ptr& decl, const std::string& filename)
{
    auto fileInfo = vfs::FileInfo("particles/", filename, vfs::Visibility::NORMAL);
    decl->setFileInfo(fileInfo);

    // Legacy invocation
    decl->setFilename(os::getFilename(fileInfo.fullPath()));
}

inline void expectParticleIsPresentInFile(const particles::IParticleDef::Ptr& decl, const std::string& path, bool expectPresent)
{
    auto contents = algorithm::loadTextFromVfsFile(path);

    parser::DefBlockSyntaxParser<const std::string> parser(contents);
    auto syntaxTree = parser.parse();

    std::vector<parser::DefBlockSyntax::Ptr> foundBlocks;
    auto particleName = string::to_lower_copy(decl->getDeclName());

    syntaxTree->foreachBlock([&](const parser::DefBlockSyntax::Ptr& block)
    {
        auto blockContents = block->getBlockContents();

        if (decl->getNumStages() > 0 && 
            block->getType() && block->getType()->getString() == "particle" &&
            block->getName() && block->getName()->getString() == particleName)
        {
            if (blockContents.find(decl->getStage(0)->getMaterialName()) == std::string::npos)
            {
                return;
            }

            foundBlocks.push_back(block);
        }
    });

    if (expectPresent)
    {
        EXPECT_EQ(foundBlocks.size(), 1) << "Expected exactly one particle " << particleName << " in the contents in the file";
    }
    else
    {
        EXPECT_TRUE(foundBlocks.empty()) << "Expected no particle " << particleName << " in the contents in the file";
    }
}

// Save a new particle to a file on disk
TEST_F(ParticlesTest, SaveNewParticleToNewFile)
{
    auto decl = createParticleFromSource("some_def");

    const auto& syntax = decl->getBlockSyntax();
    auto outputPath = _context.getTestProjectPath() + syntax.fileInfo.fullPath();
    EXPECT_FALSE(fs::exists(outputPath)) << "Output file shouldn't exist yet";

    // Auto-remove the file that is going to be written
    TemporaryFile outputFile(outputPath);

    GlobalParticlesManager().saveParticleDef(decl->getDeclName());

    EXPECT_TRUE(fs::exists(outputPath)) << "Output file should exist now";

    expectParticleIsPresentInFile(decl, decl->getBlockSyntax().fileInfo.fullPath(), true);
}

// Save the particle to a file that already exists (but doesn't contain the def)
TEST_F(ParticlesTest, SaveNewParticleToExistingFile)
{
    auto decl = createParticleFromSource("some_def346");

    setParticleFilename(decl, TEST_PARTICLE_FILE);

    // Def file should not have that particle def yet
    expectParticleIsPresentInFile(decl, decl->getBlockSyntax().fileInfo.fullPath(), false);

    GlobalParticlesManager().saveParticleDef(decl->getDeclName());

    expectParticleIsPresentInFile(decl, decl->getBlockSyntax().fileInfo.fullPath(), true);
}

// Save a particle to the same physical file that originally declared the decl
TEST_F(ParticlesTest, SaveExistingParticleToExistingFile)
{
    auto decl = GlobalParticlesManager().getDefByName("firefly_blue");

    // Swap the material name of this particle
    auto syntax = decl->getBlockSyntax();
    EXPECT_NE(syntax.contents.find("firefly_blue"), std::string::npos) << "Expected the material name in the block";
    string::replace_all(syntax.contents, "firefly_blue", "modified_material");
    decl->setBlockSyntax(syntax);

    // This modified particle should not be present
    expectParticleIsPresentInFile(decl, decl->getBlockSyntax().fileInfo.fullPath(), false);

    // Save, it should be there now
    GlobalParticlesManager().saveParticleDef(decl->getDeclName());
    expectParticleIsPresentInFile(decl, decl->getBlockSyntax().fileInfo.fullPath(), true);

    // The test fixture will restore the original file contents in TearDown
}

TEST_F(ParticlesTest, SaveChangedParticle)
{
    auto decl = GlobalParticlesManager().getDefByName("firefly_blue");

    auto blockSyntaxBeforeChange = decl->getBlockSyntax().contents;

    // Swap the material name of this particle, and set the depth hack
    decl->setDepthHack(0.56789f);
    decl->getStage(0)->setMaterialName("modified_material");

    // Block syntax should have changed by now
    auto blockSyntaxAfterChange = decl->getBlockSyntax().contents;

    EXPECT_NE(blockSyntaxBeforeChange, blockSyntaxAfterChange) << "Syntax block should have changed";

    // This modified particle should not be present in the file
    expectParticleIsPresentInFile(decl, decl->getBlockSyntax().fileInfo.fullPath(), false);

    // Save, it should be there now
    GlobalParticlesManager().saveParticleDef(decl->getDeclName());
    expectParticleIsPresentInFile(decl, decl->getBlockSyntax().fileInfo.fullPath(), true);

    // The test fixture will restore the original file contents in TearDown
}

// Save particle originally defined in a PK4 file to a new physical file that overrides the PK4
TEST_F(ParticlesTest, SaveExistingParticleToNewFileOverridingPk4)
{
    auto decl = GlobalParticlesManager().getDefByName("firefly_blue_in_pk4");

    // Swap the material name of this particle
    auto syntax = decl->getBlockSyntax();
    EXPECT_NE(syntax.contents.find("firefly_blue"), std::string::npos) << "Expected the material name in the block";
    string::replace_all(syntax.contents, "firefly_blue", "modified_material");
    decl->setBlockSyntax(syntax);

    // The overriding file should not be present
    auto outputPath = _context.getTestProjectPath() + decl->getBlockSyntax().fileInfo.fullPath();

    // Let the file be deleted when we're done here
    TemporaryFile outputFile(outputPath);
    EXPECT_FALSE(fs::exists(outputPath));

    expectParticleIsPresentInFile(decl, decl->getBlockSyntax().fileInfo.fullPath(), false);

    // Save, it should be there now
    GlobalParticlesManager().saveParticleDef(decl->getDeclName());
    expectParticleIsPresentInFile(decl, decl->getBlockSyntax().fileInfo.fullPath(), true);

    // Check if the other particle declaration is still intact in the file (use the same path to check)
    auto otherDecl = GlobalParticlesManager().getDefByName("tdm_fire_torch_in_pk4");
    EXPECT_EQ(otherDecl->getDeclFilePath(), decl->getDeclFilePath()) << "The decls should be in the same .prt file";
    expectParticleIsPresentInFile(otherDecl, decl->getBlockSyntax().fileInfo.fullPath(), true);
}

// Assumes that the syntax changes after performing the given action on the named particle
// Optionally calls the given predicate with the syntax after the change
inline void expectSyntaxChangeAfter(const std::string& defName,
    std::function<void(const particles::IParticleDef::Ptr&)> action,
    std::function<void(const std::string&)> predicate = [](const std::string&) { })
{
    auto decl = GlobalParticlesManager().getDefByName(defName);
    auto blockSyntaxBeforeChange = decl->getBlockSyntax().contents;

    action(decl);

    // Block syntax should have changed by now
    auto blockSyntaxAfterChange = decl->getBlockSyntax().contents;

    EXPECT_NE(blockSyntaxBeforeChange, blockSyntaxAfterChange) << "Syntax block should have changed";

    predicate(blockSyntaxAfterChange);
}

TEST_F(ParticlesTest, SyntaxChangeAddStage)
{
    std::size_t numStagesAfterChange = 0;
    expectSyntaxChangeAfter("firefly_blue", [&](const particles::IParticleDef::Ptr& decl)
    {
        decl->addParticleStage();
        numStagesAfterChange = decl->getNumStages();
    },
    [&](const std::string& contents)
    {
        // Expect as many opening braces as there are stages
        EXPECT_EQ(std::count(contents.begin(), contents.end(), '{'), numStagesAfterChange);
    });
}

TEST_F(ParticlesTest, SyntaxChangeRemoveStage)
{
    std::size_t numStagesAfterChange = 0;
    expectSyntaxChangeAfter("firefly_blue", [&](const particles::IParticleDef::Ptr& decl)
    {
        decl->removeParticleStage(0);
        numStagesAfterChange = decl->getNumStages();
    },
    [&](const std::string& contents)
    {
        // Expect as many opening braces as there are stages
        EXPECT_EQ(std::count(contents.begin(), contents.end(), '{'), numStagesAfterChange);
    });
}

TEST_F(ParticlesTest, SyntaxChangeSwapStages)
{
    std::size_t fireBigPositionBeforeChange = std::string::npos;
    std::size_t dustPositionBeforeChange = std::string::npos;

    expectSyntaxChangeAfter("flamejet", [&](const particles::IParticleDef::Ptr& decl)
    {
        fireBigPositionBeforeChange = decl->getBlockSyntax().contents.find("textures/particles/pfirebig2");
        dustPositionBeforeChange = decl->getBlockSyntax().contents.find("textures/particles/dust");

        decl->swapParticleStages(0, 1);
    },
    [&](const std::string& contents)
    {
        EXPECT_GT(contents.find("textures/particles/pfirebig2"), fireBigPositionBeforeChange);
        EXPECT_LT(contents.find("textures/particles/dust"), dustPositionBeforeChange);
    });
}

TEST_F(ParticlesTest, SyntaxChangeDepthHack)
{
    expectSyntaxChangeAfter("firefly_blue", [](const particles::IParticleDef::Ptr& decl)
    {
        decl->setDepthHack(0.332f);
    },
    [] (const std::string& contents)
    {
        EXPECT_NE(contents.find("depthHack\t0.332"), std::string::npos);
    });

    // Depth hack should be gone when setting it back to 0
    expectSyntaxChangeAfter("firefly_blue", [](const particles::IParticleDef::Ptr& decl)
    {
        decl->setDepthHack(0.0f);
    },
    [](const std::string& contents)
    {
        EXPECT_EQ(contents.find("depthHack"), std::string::npos);
    });
}

TEST_F(ParticlesTest, SyntaxChangeStageMaterial)
{
    expectSyntaxChangeAfter("firefly_blue", [](const particles::IParticleDef::Ptr& decl)
    {
        decl->getStage(0)->setMaterialName("textures/common/caulk");
    },
    [](const std::string& contents)
    {
        EXPECT_NE(contents.find("textures/common/caulk"), std::string::npos);
    });
}

TEST_F(ParticlesTest, SyntaxChangeStageParameterChange)
{
    expectSyntaxChangeAfter("firefly_blue", [](const particles::IParticleDef::Ptr& decl)
    {
        decl->getStage(0)->getSize().setFrom(0.77f);
        decl->getStage(0)->getSize().setTo(566.22f);
    },
    [](const std::string& contents)
    {
        EXPECT_NE(contents.find("\"0.770\" to \"566.220\""), std::string::npos);
    });
}

// Acquiring a particle node with or without .prt as the name suffix
TEST_F(ParticlesTest, AcquireParticleNode)
{
    EXPECT_TRUE(GlobalParticlesManager().createParticleNode("firefly_blue_in_pk4.prt"));
    EXPECT_TRUE(GlobalParticlesManager().createParticleNode("firefly_blue_in_pk4"));
}

}
