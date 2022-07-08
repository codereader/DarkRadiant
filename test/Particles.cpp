#include "RadiantTest.h"

#include "iparticles.h"
#include "iparticlestage.h"

namespace test
{

using ParticlesTest = RadiantTest;

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
    EXPECT_EQ(decl->getStage(0).getMaterialName(), "textures/common/caulk")
        << "The particle using the caulk texture should have taken precedence";
}

// Prove that the ignored, duplicate particleDef is not stopping the parser from processing the rest of the file
TEST_F(ParticlesTest, ParticleBelowDuplicatedIsParsed)
{
    auto decl = GlobalParticlesManager().getDefByName("particle_after_precedencecheck");

    EXPECT_TRUE(decl) << "Could not locate the particleDef that should go below the precedencecheck in z_precedence.prt";
    EXPECT_EQ(decl->getStage(0).getMaterialName(), "textures/common/nodraw")
        << "The particle using the caulk texture should have taken precedence";
}

TEST_F(ParticlesTest, FindParticle)
{
    EXPECT_TRUE(GlobalParticlesManager().getDefByName("tdm_fire_torch")) << "tdm_fire_torch not found";
    EXPECT_TRUE(GlobalParticlesManager().getDefByName("firefly_blue")) << "firefly_blue not found";
    EXPECT_TRUE(GlobalParticlesManager().getDefByName("firefly_yellow")) << "mixed case typename particle firefly_yellow not found";
    EXPECT_TRUE(GlobalParticlesManager().getDefByName("firefly_green")) << "particle without typename firefly_green not found";
    EXPECT_TRUE(GlobalParticlesManager().getDefByName("flamejet"));

    EXPECT_FALSE(GlobalParticlesManager().getDefByName("flamejet_nonexisting")) << "flamejet_nonexisting doesn't exist";
}

TEST_F(ParticlesTest, ParticleMetadata)
{
    auto decl = GlobalParticlesManager().getDefByName("flamejet");

    EXPECT_EQ(decl->getDeclName(), "flamejet");
    EXPECT_EQ(decl->getModName(), RadiantTest::DefaultGameType);
    EXPECT_EQ(decl->getDeclFilePath(), "particles/testparticles.prt");
}

TEST_F(ParticlesTest, ParticleProperties)
{
    auto decl = GlobalParticlesManager().getDefByName("flamejet");

    EXPECT_EQ(decl->getDepthHack(), 0.001f);
    EXPECT_EQ(decl->getNumStages(), 3);

    EXPECT_EQ(decl->getStage(0).getCount(), 20);
    EXPECT_EQ(decl->getStage(0).getMaterialName(), "textures/particles/pfirebig2");
    EXPECT_EQ(decl->getStage(0).getDuration(), 0.7f);
    EXPECT_EQ(decl->getStage(0).getCycles(), 0.0f);
    EXPECT_EQ(decl->getStage(0).getBunching(), 1.0f);
    EXPECT_EQ(decl->getStage(0).getDistributionType(), particles::IStageDef::DISTRIBUTION_CYLINDER);
    EXPECT_EQ(decl->getStage(0).getDistributionParm(0), 4);
    EXPECT_EQ(decl->getStage(0).getDistributionParm(1), 4);
    EXPECT_EQ(decl->getStage(0).getDistributionParm(2), 10);
    EXPECT_EQ(decl->getStage(0).getDistributionParm(3), 0);
    EXPECT_EQ(decl->getStage(0).getDirectionType(), particles::IStageDef::DIRECTION_CONE);
    EXPECT_EQ(decl->getStage(0).getDirectionParm(0), 10.0);
    EXPECT_EQ(decl->getStage(0).getOrientationType(), particles::IStageDef::ORIENTATION_VIEW);
    EXPECT_EQ(decl->getStage(0).getSpeed().getFrom(), 86);
    EXPECT_EQ(decl->getStage(0).getSpeed().getTo(), -4);
    EXPECT_EQ(decl->getStage(0).getSize().getFrom(), 16.5f);
    EXPECT_EQ(decl->getStage(0).getSize().getTo(), 28.5f);
    EXPECT_EQ(decl->getStage(0).getAspect().getFrom(), 1.0f);
    EXPECT_EQ(decl->getStage(0).getAspect().getTo(), 1.0f);
    EXPECT_EQ(decl->getStage(0).getRotationSpeed().getFrom(), 24.0f);
    EXPECT_EQ(decl->getStage(0).getRotationSpeed().getTo(), 29.0f);
    EXPECT_EQ(decl->getStage(0).getFadeInFraction(), 0.35f);
    EXPECT_EQ(decl->getStage(0).getFadeOutFraction(), 0.2f);
    EXPECT_EQ(decl->getStage(0).getColour(), Vector4(0.92, 0.92, 0.92, 1));
    EXPECT_EQ(decl->getStage(0).getFadeColour(), Vector4(0, 0, 0, 1));
    EXPECT_EQ(decl->getStage(0).getOffset(), Vector3(0, 0, 0));
    EXPECT_EQ(decl->getStage(0).getGravity(), 10.0f);
    EXPECT_EQ(decl->getStage(0).getWorldGravityFlag(), false);

    EXPECT_EQ(decl->getStage(1).getCount(), 4);
    EXPECT_EQ(decl->getStage(1).getMaterialName(), "textures/particles/dust");
    EXPECT_EQ(decl->getStage(1).getDuration(), 1.6f);
    EXPECT_EQ(decl->getStage(1).getCycles(), 0.0f);
    EXPECT_EQ(decl->getStage(1).getBunching(), 1.0f);
    EXPECT_EQ(decl->getStage(1).getDistributionType(), particles::IStageDef::DISTRIBUTION_CYLINDER);
    EXPECT_EQ(decl->getStage(1).getDistributionParm(0), 2);
    EXPECT_EQ(decl->getStage(1).getDistributionParm(1), 2);
    EXPECT_EQ(decl->getStage(1).getDistributionParm(2), 2);
    EXPECT_EQ(decl->getStage(1).getDistributionParm(3), 0);
    EXPECT_EQ(decl->getStage(1).getDirectionType(), particles::IStageDef::DIRECTION_CONE);
    EXPECT_EQ(decl->getStage(1).getDirectionParm(0), 90.0);
    EXPECT_EQ(decl->getStage(1).getOrientationType(), particles::IStageDef::ORIENTATION_VIEW);
    EXPECT_EQ(decl->getStage(1).getSpeed().getFrom(), 47);
    EXPECT_EQ(decl->getStage(1).getSpeed().getTo(), 0);
    EXPECT_EQ(decl->getStage(1).getSize().getFrom(), 13.0f);
    EXPECT_EQ(decl->getStage(1).getSize().getTo(), 38.5f);
    EXPECT_EQ(decl->getStage(1).getAspect().getFrom(), 1.5f);
    EXPECT_EQ(decl->getStage(1).getAspect().getTo(), 1.0f);
    EXPECT_EQ(decl->getStage(1).getRotationSpeed().getFrom(), 50.0f);
    EXPECT_EQ(decl->getStage(1).getRotationSpeed().getTo(), 85.0f);
    EXPECT_EQ(decl->getStage(1).getFadeInFraction(), 1.0f);
    EXPECT_EQ(decl->getStage(1).getFadeOutFraction(), 0.6f);
    EXPECT_EQ(decl->getStage(1).getColour(), Vector4(1, 1, 1, 1));
    EXPECT_EQ(decl->getStage(1).getFadeColour(), Vector4(0, 0, 1, 0));
    EXPECT_EQ(decl->getStage(1).getOffset(), Vector3(0, 0, 4));
    EXPECT_EQ(decl->getStage(1).getGravity(), -29.0f);
    EXPECT_EQ(decl->getStage(1).getWorldGravityFlag(), true);

    // Test a few key properties of stage #3 that are not covered by stages #1 and #2
    EXPECT_EQ(decl->getStage(2).getDistributionType(), particles::IStageDef::DISTRIBUTION_RECT);
    EXPECT_EQ(decl->getStage(2).getDistributionParm(0), 3);
    EXPECT_EQ(decl->getStage(2).getDistributionParm(1), 3);
    EXPECT_EQ(decl->getStage(2).getDistributionParm(2), 0);
    EXPECT_EQ(decl->getStage(2).getDirectionType(), particles::IStageDef::DIRECTION_OUTWARD);
    EXPECT_EQ(decl->getStage(2).getDirectionParm(0), 100.0);
    EXPECT_EQ(decl->getStage(2).getRotationSpeed().getFrom(), 100.0f);
    EXPECT_EQ(decl->getStage(2).getRotationSpeed().getTo(), 100.0f);
    EXPECT_EQ(decl->getStage(2).getOffset(), Vector3(0, 0, -20));
}

}
