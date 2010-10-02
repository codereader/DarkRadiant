#ifndef PARTICLEDEF_H_
#define PARTICLEDEF_H_

#include "ParticleStage.h"

#include "iparticles.h"

#include <vector>

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
};
typedef boost::shared_ptr<ParticleDef> ParticleDefPtr;

}

#endif /*PARTICLEDEF_H_*/
