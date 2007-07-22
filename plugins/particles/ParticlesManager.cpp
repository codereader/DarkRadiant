#include "ParticlesManager.h"
#include "ParticleFileLoader.h"
#include "ParticleDef.h"
#include "ParticleStage.h"

#include "ifilesystem.h"

#include "generic/callback.h"
#include "parser/DefTokeniser.h"

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
	
	// Usual ritual, get a parser::DefTokeniser and start tokenising the DEFs
	parser::DefTokeniser tok(contents);
	
	while (tok.hasMoreTokens()) {
		parseParticleDef(tok);
	}
}

// Parse a single particle def
void ParticlesManager::parseParticleDef(parser::DefTokeniser& tok) {

	// Standard DEF, starts with "particle <name> {"
	tok.assertNextToken("particle");
	
	ParticleDef pdef(tok.nextToken());
	tok.assertNextToken("{");
	
	// Any global keywords will come first, after which we get a series of 
	// brace-delimited stages.
	std::string token = tok.nextToken();
	while (token != "}") {
		if (token == "depthHack") {
			tok.skipTokens(1); // we don't care about depthHack
		}
		else if (token == "{") {
			// Start of stage
			ParticleStage stage(parseParticleStage(tok));
		}
		
		// Get next token
		token = tok.nextToken();
	}
}

// Parse an individual particle stage
ParticleStage ParticlesManager::parseParticleStage(parser::DefTokeniser& tok) {
	while (tok.hasMoreTokens() && tok.nextToken() != "}") { }
	return ParticleStage();
}

}
