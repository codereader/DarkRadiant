#include "ParticlesManager.h"

#include "ParticleFileLoader.h"
#include "ParticleDef.h"
#include "ParticleStage.h"
#include "RenderableParticle.h"

#include "icommandsystem.h"
#include "ieventmanager.h"
#include "ifilesystem.h"

#include "parser/DefTokeniser.h"
#include "math/Vector4.h"

#include "debugging/ScopedDebugTimer.h"

#include "string/string.h"
#include <iostream>
#include <boost/bind.hpp>

namespace particles {

void ParticlesManager::addObserver(IParticlesManager::Observer* observer)
{
	_observers.insert(observer);
}

void ParticlesManager::removeObserver(IParticlesManager::Observer* observer)
{
	_observers.erase(observer);
}

// Visit all of the particle defs
void ParticlesManager::forEachParticleDef(const ParticleDefVisitor& v) const
{
	// Invoke the visitor for each ParticleDef object
	for (ParticleDefMap::const_iterator i = _particleDefs.begin();
		 i != _particleDefs.end();
		 ++i)
	{
		v(*i->second);
	}
}

IParticleDefPtr ParticlesManager::getParticle(const std::string& name)
{
	ParticleDefMap::const_iterator found = _particleDefs.find(name);

	return (found != _particleDefs.end()) ? found->second : IParticleDefPtr();
}

IRenderableParticlePtr ParticlesManager::getRenderableParticle(const std::string& name)
{
	ParticleDefMap::const_iterator found = _particleDefs.find(name);

	if (found != _particleDefs.end()) 
	{
		return RenderableParticlePtr(new RenderableParticle(found->second));
	}
	else
	{
		return IRenderableParticlePtr();
	}
}

ParticleDefPtr ParticlesManager::findOrInsertParticleDef(const std::string& name)
{
	ParticleDefMap::iterator i = _particleDefs.find(name);

	if (i != _particleDefs.end())
	{
		// Particle def is already existing in the map
		return i->second;
	}

	// Not existing, add a new ParticleDef to the map
	std::pair<ParticleDefMap::iterator, bool> result = _particleDefs.insert(
		ParticleDefMap::value_type(name, ParticleDefPtr(new ParticleDef(name))));

	// Return the iterator from the insertion result
	return result.first->second;
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
	
	ParticleDefPtr pdef = findOrInsertParticleDef(name);

	// Clear out the particle def before parsing
	pdef->clear();

	// Any global keywords will come first, after which we get a series of 
	// brace-delimited stages.
	std::string token = tok.nextToken();

	while (token != "}")
	{
		if (token == "depthHack")
		{
			pdef->setDepthHack(strToFloat(tok.nextToken()));
		}
		else if (token == "{")
		{
			// Construct/Parse the stage from the tokens
			ParticleStage stage(tok);
			
			// Append to the ParticleDef
			pdef->appendStage(stage);
		}
		
		// Get next token
		token = tok.nextToken();
	}
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
		_dependencies.insert(MODULE_COMMANDSYSTEM);
		_dependencies.insert(MODULE_EVENTMANAGER);
	}

	return _dependencies;
}

void ParticlesManager::initialiseModule(const ApplicationContext& ctx)
{
	globalOutputStream() << "ParticlesManager::initialiseModule called" << std::endl;
	
	// Load the .prt files
	reloadParticleDefs();

	// Register the "ReloadParticles" commands
	GlobalCommandSystem().addCommand("ReloadParticles", boost::bind(&ParticlesManager::reloadParticleDefs, this));
	GlobalEventManager().addCommand("ReloadParticles", "ReloadParticles");
}

void ParticlesManager::reloadParticleDefs()
{
	// Use a ParticleFileLoader to load each file
	ParticleFileLoader loader(*this);

	ScopedDebugTimer timer("Particle definitions parsed: ");
	GlobalFileSystem().forEachFile(PARTICLES_DIR, PARTICLES_EXT, loader, 1);

	// Notify observers about this event
	for (Observers::const_iterator i = _observers.begin(); i != _observers.end();)
	{
		(*i++)->onParticleDefReload();
	}
}

} // namespace particles
