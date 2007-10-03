#include "ParticlesManager.h"
#include "ParticleFileLoader.h"
#include "ParticleDef.h"
#include "ParticleStage.h"

#include "ifilesystem.h"
#include "stream/textstream.h"

#include "generic/callback.h"
#include "parser/DefTokeniser.h"
#include "math/Vector4.h"

#include <boost/lexical_cast.hpp>
#include <iostream>

namespace particles {

// Visit all of the particle defs
void ParticlesManager::forEachParticleDef(const ParticleDefVisitor& v) const
{
	// Invoke the visitor for each ParticleDef object
	for (ParticleDefMap::const_iterator i = _particleDefs.begin();
		 i != _particleDefs.end();
		 ++i)
	{
		v(i->second);
	}
}

// Parse particle defs from string
void ParticlesManager::parseStream(std::istream& contents) {
	
	// Usual ritual, get a parser::DefTokeniser and start tokenising the DEFs
	parser::BasicDefTokeniser<std::istream> tok(contents);
	
	while (tok.hasMoreTokens()) {
		parseParticleDef(tok);
	}
}

// Parse a single particle def
void ParticlesManager::parseParticleDef(parser::DefTokeniser& tok) {

	// Standard DEF, starts with "particle <name> {"
	tok.assertNextToken("particle");
	std::string name = tok.nextToken();
	tok.assertNextToken("{");
	
	ParticleDef pdef(name);

	// Any global keywords will come first, after which we get a series of 
	// brace-delimited stages.
	std::string token = tok.nextToken();
	while (token != "}") {
		if (token == "depthHack") {
			tok.skipTokens(1); // we don't care about depthHack
		}
		else if (token == "{") {
			
			// Parse stage
			ParticleStage stage(parseParticleStage(tok));
			
			// Append to the ParticleDef
			pdef.appendStage(stage);
		}
		
		// Get next token
		token = tok.nextToken();
	}
	
	// Add the ParticleDef to the map
	_particleDefs.insert(ParticleDefMap::value_type(name, pdef));
}

// Parse an individual particle stage
ParticleStage ParticlesManager::parseParticleStage(parser::DefTokeniser& tok) {
	
	ParticleStage stage;
	
	// Read values. These are not a simple list of keyvalue pairs, but some
	// values may consist of more than one token.
	std::string token = tok.nextToken();
	while (token != "}") {
		
		if (token == "count") {
			try {
				stage.setCount(boost::lexical_cast<int>(tok.nextToken()));
			}
			catch (boost::bad_lexical_cast e) {
				std::cerr << "[particles] Bad count value '" << token 
						  << "'" << std::endl;
			}
		}
		else if (token == "color") {
			
			// Read 4 values and assemble as a vector4
			Vector4 col;
			col.x() = boost::lexical_cast<float>(tok.nextToken());
			col.y() = boost::lexical_cast<float>(tok.nextToken());
			col.z() = boost::lexical_cast<float>(tok.nextToken());
			col.w() = boost::lexical_cast<float>(tok.nextToken());
			
			// Set the stage colour
			stage.setColour(col);
		}
		
		token = tok.nextToken();
	}
	
	return stage;
}

const std::string& ParticlesManager::getName() const {
	static std::string _name(MODULE_PARTICLESMANAGER);
	return _name;
}

const StringSet& ParticlesManager::getDependencies() const {
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_VIRTUALFILESYSTEM);
	}

	return _dependencies;
}

void ParticlesManager::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "ParticlesManager::initialiseModule called\n";
	
	// Use a ParticleFileLoader to load each file
	ParticleFileLoader loader(*this);
	GlobalFileSystem().forEachFile(
		PARTICLES_DIR, 
		PARTICLES_EXT,
		makeCallback1(loader),
		1
	);
}

} // namespace particles
