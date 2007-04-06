#ifndef COMPONENT_H_
#define COMPONENT_H_

#include <string>

namespace objectives
{

/**
 * A component of an objective. Each objective may have a number of components,
 * which are combined using boolean operations to determine whether the
 * objective is satisfied or not.
 */

class Component
{
public:

	// Is this component satisfied?
	bool state;
	
	// Invert the sense of this component
	bool inverted;
	
	// Component changes state once then latches
	bool irreversible;
	
	// The type of this component ("kill", "ko" etc)
	std::string type;
	
	// Default constructor
	Component()
	: state(false), inverted(false), irreversible(false), type("")
	{ }
};

}

#endif /*COMPONENT_H_*/
