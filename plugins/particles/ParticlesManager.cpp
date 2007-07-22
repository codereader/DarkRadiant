#include "ParticlesManager.h"

#include <iostream>

namespace particles
{

void ParticlesManager::forEachParticleDef(const ParticleDefVisitor& v) const
{
	std::cout << "Particles visit" << std::endl;
}

}
