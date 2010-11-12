#ifndef PARTICLEDEF_H_
#define PARTICLEDEF_H_

#include "ParticleStage.h"

#include "iparticles.h"

#include "parser/DefTokeniser.h"
#include "string/string.h"
#include <vector>
#include <set>

namespace particles
{

/**
 * Representation of a single particle definition. Each definition is comprised
 * of a number of "stages", which must all be rendered in turn.
 */
class ParticleDef
: public IParticleDef
{
	// Name
	std::string _name;

	// Depth hack
	float _depthHack;

	// Vector of stages
	typedef std::vector<ParticleStage> StageList;
	StageList _stages;

	typedef std::set<IParticleDef::Observer*> Observers;
	Observers _observers;

public:

	/**
	 * Construct a named ParticleDef.
	 */
	ParticleDef(const std::string& name)
	: _name(name)
	{ }

	/**
	 * Return the ParticleDef name.
	 */
	std::string getName() const {
		return _name;
	}

	// Clears stage and depth hack information
	// Name and observers are NOT cleared
	void clear()
	{
		_depthHack = false;
		_stages.clear();
	}

	float getDepthHack() const
	{
		return _depthHack;
	}

	void setDepthHack(float value)
	{
		_depthHack = value;
	}

	/**
	 * Returns the number of stages for this particle system.
	 */
	std::size_t getNumStages()
	{
		return _stages.size();
	}

	/**
	 * Return a specific particle stage (const version)
	 */
	const IParticleStage& getParticleStage(std::size_t stageNum) const
	{
		return _stages[stageNum];
	}

	/**
	 * Return a specific particle stage (non-const version)
	 */
	IParticleStage& getParticleStages(std::size_t stageNum)
	{
		return _stages[stageNum];
	}

	/**
	 * Append a particle stage.
	 */
	void appendStage(const ParticleStage& stage) {
		_stages.push_back(stage);
	}

	void addObserver(IParticleDef::Observer* observer)
	{
		_observers.insert(observer);
	}

	void removeObserver(IParticleDef::Observer* observer)
	{
		_observers.erase(observer);
	}

	void parseFromTokens(parser::DefTokeniser& tok)
	{
		// Clear out the particle def (except the name) before parsing
		clear();

		// Any global keywords will come first, after which we get a series of
		// brace-delimited stages.
		std::string token = tok.nextToken();

		while (token != "}")
		{
			if (token == "depthHack")
			{
				setDepthHack(strToFloat(tok.nextToken()));
			}
			else if (token == "{")
			{
				// Construct/Parse the stage from the tokens
				ParticleStage stage(tok);

				// Append to the ParticleDef
				appendStage(stage);
			}

			// Get next token
			token = tok.nextToken();
		}

		// Notify any observers about this event
		for (Observers::const_iterator i = _observers.begin(); i != _observers.end();)
		{
			(*i++)->onParticleReload();
		}
	}
};
typedef boost::shared_ptr<ParticleDef> ParticleDefPtr;

}

#endif /*PARTICLEDEF_H_*/
