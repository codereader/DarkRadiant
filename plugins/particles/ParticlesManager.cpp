#include "ParticlesManager.h"
#include "ParticleFileLoader.h"
#include "ParticleDef.h"
#include "ParticleStage.h"

#include "ifilesystem.h"

#include "parser/DefTokeniser.h"
#include "math/Vector4.h"

#include "debugging/ScopedDebugTimer.h"

#include "string/string.h"
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
void ParticlesManager::parseStream(std::istream& contents)
{
	// Usual ritual, get a parser::DefTokeniser and start tokenising the DEFs
	parser::BasicDefTokeniser<std::istream> tok(contents);
	
	while (tok.hasMoreTokens())
	{
		parseParticleDef(tok);
	}
}

// Parse a single particle def
void ParticlesManager::parseParticleDef(parser::DefTokeniser& tok)
{
	// Standard DEF, starts with "particle <name> {"
	std::string declName = tok.nextToken();

	// Check for a valid particle declaration, some .prt files contain materials
	if (declName != "particle")
	{
		// No particle, skip name plus whole block
		tok.skipTokens(1);
		tok.assertNextToken("{");

		for (std::size_t level = 1; level > 0;)
		{
			std::string token = tok.nextToken();

			if (token == "}")
			{
				level--;
			}
			else if (token == "{")
			{
				level++;
			}
		}

		return;
	}

	// Valid particle declaration, go ahead parsing the name
	std::string name = tok.nextToken();
	tok.assertNextToken("{");
	
	ParticleDef pdef(name);

	// Any global keywords will come first, after which we get a series of 
	// brace-delimited stages.
	std::string token = tok.nextToken();

	while (token != "}")
	{
		if (token == "depthHack")
		{
			pdef.setDepthHack(strToFloat(tok.nextToken()));
		}
		else if (token == "{")
		{
			// Construct/Parse the stage from the tokens
			ParticleStage stage(tok);
			
			// Append to the ParticleDef
			pdef.appendStage(stage);
		}
		
		// Get next token
		token = tok.nextToken();
	}
	
	// Add the ParticleDef to the map
	_particleDefs.insert(ParticleDefMap::value_type(name, pdef));
}

const std::string& ParticlesManager::getName() const
{
	static std::string _name(MODULE_PARTICLESMANAGER);
	return _name;
}

const StringSet& ParticlesManager::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_VIRTUALFILESYSTEM);
	}

	return _dependencies;
}

void ParticlesManager::initialiseModule(const ApplicationContext& ctx)
{
	globalOutputStream() << "ParticlesManager::initialiseModule called" << std::endl;
	
	// Use a ParticleFileLoader to load each file
	ParticleFileLoader loader(*this);

	ScopedDebugTimer timer("Particle definitions parsed: ");
	GlobalFileSystem().forEachFile(PARTICLES_DIR, PARTICLES_EXT, loader, 1);
}

} // namespace particles
