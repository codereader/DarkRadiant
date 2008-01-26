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

	/**
	 * Flag to indicate that this component has been satisfied.
	 */
	bool state;
	
	/**
	 * Flag to indicate that the sense of this component is inverted.
	 */
	bool inverted;
	
	/**
	 * Flag to indicate that this component changes state once then latches,
	 * even if its in-game condition is no longer satisfied.
	 */
	bool irreversible;
	
	/**
	 * The type of this component ("kill", "ko" etc).
	 */
	std::string type;
	
	/**
	 * Construct a Component with default settings. All flags are set to false,
	 * and the type is set to the empty string.
	 */
	Component()
	: state(false), inverted(false), irreversible(false), type("")
	{ }
};

}

#endif /*COMPONENT_H_*/
