#ifndef COMPONENT_H_
#define COMPONENT_H_

#include "SpecifierType.h"
#include "Specifier.h"
#include "ComponentType.h"

#include <cassert>
#include <vector>
#include <string>
#include <boost/algorithm/string/join.hpp>

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
private:
	// Completion state flag
	bool _satisfied;

	// Inverted flag
	bool _inverted;

	// Irreversible (latched) flag
	bool _irreversible;

	// Player responsible flag
	bool _playerResponsible;

	// The clock interval in seconds (only applicable for clocked components)
	// is negative if not used
	float _clockInterval;

	// Component type
	ComponentType _type;

    // List of Specifiers (SpecifierType + value pairs)
    SpecifierList _specifiers;

	/**
	 * greebo: Each component can have an arbitrary number of arguments.
	 */
	typedef std::vector<std::string> ArgumentList;
	ArgumentList _arguments;

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
	  _clockInterval(-1.0f),
	  _type(ComponentType::COMP_KILL()), // arbitrary choice, no NONE option
      _specifiers(static_cast<std::size_t>(Specifier::MAX_SPECIFIERS))
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
     * Get the player-responsible status.
     */
    bool isPlayerResponsible() const {
        return _playerResponsible;
    }

	/**
     * Set the clock interval for clocked components in seconds.
     */
	void setClockInterval(float clockInterval) {
		_clockInterval = clockInterval;
	}

	/**
     * Get the clock interval for clocked components in seconds.
	 * Returns negative values if this option is not used for this component.
     */
	float getClockInterval() const {
		return _clockInterval;
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
	std::string getString();

    /**
     * Set the Specifier at the given index.
     *
     * @param idx
     * The index of the specifier, starting from 1.
     */
    void setSpecifier(Specifier::SpecifierNumber num, SpecifierPtr spec)
    {
        assert(
            _specifiers.size() == static_cast<std::size_t>(
                Specifier::MAX_SPECIFIERS
            )
        );
        _specifiers[static_cast<std::size_t>(num)] = spec;
    }

    /**
     * Get the Specifier at the given index.
     *
     * @param idx
     * The index of the specifier, starting from 1.
     */
    SpecifierPtr getSpecifier(Specifier::SpecifierNumber num) const
    {
        assert(
            _specifiers.size() == static_cast<std::size_t>(
                Specifier::MAX_SPECIFIERS
            )
        );
        return _specifiers[static_cast<std::size_t>(num)];
    }

	void clearArguments() {
		_arguments.clear();
	}

	void addArgument(const std::string& arg) {
		_arguments.push_back(arg);
	}

	std::size_t getNumArguments() {
		return _arguments.size();
	}

	// returns "" if the argument with the given index doesn't exist
	std::string getArgument(std::size_t index) {
		return index < _arguments.size() ? _arguments[index] : "";
	}

	// Sets the argument with the given index
	void setArgument(std::size_t index, const std::string& value) {
		// Ensure that the vector is large enough
		if (_arguments.size() <= index) {
			_arguments.resize(index+1);
		}

		// Now set the value
		_arguments[index] = value;
	}

	// Returns all arguments in a space-delimited string
	std::string getArgumentString() const {
		return boost::algorithm::join(_arguments, " ");
	}
};

}

#endif /*COMPONENT_H_*/
