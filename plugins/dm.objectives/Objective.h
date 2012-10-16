#ifndef OBJECTIVE_H_
#define OBJECTIVE_H_

#include "Component.h"
#include "Logic.h"

#include "i18n.h"
#include <string>
#include <map>
#include <list>

namespace objectives
{

/**
 * Data object representing a single mission objective.
 */
class Objective
{
public:
	// Description of objective
	std::string description;

	// Objective state enum
	enum State {
		INCOMPLETE = 0,
		COMPLETE = 1,
		INVALID = 2,
		FAILED = 3,
		NUM_STATES
	} state;

	// Boolean flags
	bool mandatory;
	bool visible;
	bool ongoing;
	bool irreversible;

	// The difficulty levels this objective applies to (space-delimited integers)
	std::string difficultyLevels;

	// The prerequisites of this objective (a space-delimited index list for now)
	std::string enablingObjs;

	// Success/failure logic
	Logic logic;

	// The scripts that are called when this objective completes/fails
	std::string completionScript;
	std::string failureScript;

	// These entities are triggered when this objective completes/fails
	std::string completionTarget;
	std::string failureTarget;

	// Map of indexed components
	typedef std::map<int, Component> ComponentMap;
	ComponentMap components;

	// Constructor
	Objective()	:
		state(INCOMPLETE),
		mandatory(true),
		visible(true),
		ongoing(false),
		irreversible(false)
	{}

	static std::string getStateText(State state)
	{
		switch (state)
		{
			case INCOMPLETE: return _("INCOMPLETE");
			case COMPLETE: return _("COMPLETE");
			case FAILED: return _("FAILED");
			case INVALID: return _("INVALID");
			default: return "-";
		};
	}
};

/**
 * Objective map type.
 */
typedef std::map<int, Objective> ObjectiveMap;

}

#endif /*OBJECTIVE_H_*/
