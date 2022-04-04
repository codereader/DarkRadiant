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

}
