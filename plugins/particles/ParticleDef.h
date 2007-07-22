#ifndef PARTICLEDEF_H_
#define PARTICLEDEF_H_

#include "iparticles.h"

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
};

}

#endif /*PARTICLEDEF_H_*/
