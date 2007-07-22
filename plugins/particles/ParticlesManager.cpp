#include "ParticlesManager.h"
#include "ParticleFileLoader.h"

#include "ifilesystem.h"
#include "generic/callback.h"

#include <iostream>

namespace particles
{

// Main constructor reads the .prt files
ParticlesManager::ParticlesManager() {
	
	// Use a ParticleFileLoader to load each file
	ParticleFileLoader loader(*this);
	GlobalFileSystem().forEachFile(
		PARTICLES_DIR, 
		PARTICLES_EXT,
		makeCallback1(loader),
		1
	);
	
}

void ParticlesManager::forEachParticleDef(const ParticleDefVisitor& v) const
{
	std::cout << "Particles visit" << std::endl;
}

// Parse particle defs from string
void ParticlesManager::parseString(const std::string& contents) {
	//std::cout << "Parsing" << std::endl;
}

}
