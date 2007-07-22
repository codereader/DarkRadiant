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
	
	/**
	 * Append a particle stage.
	 */
	void appendStage(const ParticleStage& stage) {
		_stages.push_back(stage);
	}
};

}

#endif /*PARTICLEDEF_H_*/
