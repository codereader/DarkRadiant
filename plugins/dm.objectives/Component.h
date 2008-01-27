#ifndef COMPONENT_H_
#define COMPONENT_H_

#include "Specifier.h"

#include <string>

namespace objectives
{

/**
 * A component of an objective. 
 * 
 * Each objective may have a number of components, which are combined using 
 * boolean operations to determine whether the objective is satisfied or not.
 * A component is essentially a condition which needs to be met in order to
 * satisfy the overall objective.
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
	 * 
	 * If true, this component is logically <b>NOT</b>ed, so when the conditions 
	 * described by the type and specifiers are not met, the component state is 
	 * true, and when they are met, it is false.
	 */
	bool inverted;
	
	/**
	 * Flag to indicate that this component changes state once then latches,
	 * even if its in-game condition is no longer satisfied.
	 */
	bool irreversible;
	
	/**
	 * Flag to indicate that the player must be responsible for satisfying this
	 * component.
	 * 
	 * If this flag is set, the component will not be satisfied by an event
	 * which is not <i>directly</i> caused by the player. For example, if the
	 * component requires the killing of an AI, it will not be satisfied if the
	 * AI is killed by another AI rather than the player.
	 */
	bool playerResponsible;
	
	/**
	 * The type of this component ("kill", "ko" etc).
	 */
	std::string type;
	
	/**
	 * The first specifier type.
	 */
	Specifier spec1_type;
	
	/**
	 * The first specifier value.
	 */
	std::string spec1_val;
	
	/**
	 * The second specifier type.
	 */
	Specifier spec2_type;
	
	/**
	 * The second specifier value.
	 */
	std::string spec2_val;
	
	/**
	 * Construct a Component with default settings. All flags are set to false,
	 * and the type is set to the empty string.
	 */
	Component()
	: state(false), inverted(false), irreversible(false), type(""),
	  spec1_type(Specifier::SPEC_NONE),
	  spec2_type(Specifier::SPEC_NONE)
	{ }
};

}

#endif /*COMPONENT_H_*/
