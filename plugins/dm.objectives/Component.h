#ifndef COMPONENT_H_
#define COMPONENT_H_

#include "Specifier.h"
#include "ComponentType.h"

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
	// Completion state flag
	bool _satisfied;
	
	// Inverted flag
	bool _inverted;
	
	// Irreversible (latched) flag
	bool _irreversible;
	
	// Player responsible flag
	bool _playerResponsible;
	
	// Component type
	ComponentType _type;
	
public:
	
	/**
	 * Construct a Component with default settings. 
	 * 
	 * All flags are set to false, and the type is set to an arbitrary value.
	 */
	Component()
	: _satisfied(false), 
	  _inverted(false), 
	  _irreversible(false), 
	  _playerResponsible(false), 
	  _type(ComponentType::COMP_KILL()) // arbitrary choice, no NONE option
	{ }

	/**
	 * Set the flag to indicate that this component has been satisfied.
	 */
	void setSatisfied(bool satisfied) {
		_satisfied = satisfied;
	}
	
	/**
	 * Get the satisfied status flag.
	 */
	bool isSatisfied() const {
		return _satisfied;
	}
	
	/**
	 * Set the flag to indicate that the sense of this component is inverted.
	 * 
	 * If true, this component is logically <b>NOT</b>ed, so when the conditions 
	 * described by the type and specifiers are not met, the component state is 
	 * true, and when they are met, it is false.
	 */
	void setInverted(bool inverted) {
		_inverted = inverted;
	}
	
	/**
	 * Get the inverted status.
	 */
	bool isInverted() const {
		return _inverted;
	}
	
	/**
	 * Set the flag to indicate that this component changes state once then 
	 * latches, even if its in-game condition is no longer satisfied.
	 */
	void setIrreversible(bool irreversible) {
		_irreversible = irreversible;
	}
	
	/**
	 * Get the irreversible status.
	 */
	bool isIrreversible() const {
		return _irreversible;
	}
	
	/**
	 * Set the flag to indicate that the player must be responsible for 
	 * satisfying this component.
	 * 
	 * If this flag is set, the component will not be satisfied by an event
	 * which is not <i>directly</i> caused by the player. For example, if the
	 * component requires the killing of an AI, it will not be satisfied if the
	 * AI is killed by another AI rather than the player.
	 */
	void setPlayerResponsible(bool playerResponsible) {
		_playerResponsible = playerResponsible;
	}
	
	/**
	 * Set the type of this component ("kill", "ko" etc).
	 */
	void setType(ComponentType type) {
		_type = type;
	}
	
	/**
	 * Get the component type.
	 */
	ComponentType getType() const {
		return _type;
	}
	
	/**
	 * Return a string description of this Component.
	 */
	std::string getString() const {
		return _type.getDisplayName();
	}
};

}

#endif /*COMPONENT_H_*/
