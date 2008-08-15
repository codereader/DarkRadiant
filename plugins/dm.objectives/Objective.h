#ifndef OBJECTIVE_H_
#define OBJECTIVE_H_

#include "Component.h"
#include "Logic.h"

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
		COMPLETE,
		INCOMPLETE,
		FAILED,
		INVALID
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
	
	// Map of indexed components
	typedef std::map<int, Component> ComponentMap;
	ComponentMap components;
	
	// Constructor
	Objective()	: 
		state(INCOMPLETE), 
		mandatory(false), 
		visible(false), 
		ongoing(false),
		irreversible(false)
	{}
};

/**
 * Objective map type.
 */
typedef std::map<int, Objective> ObjectiveMap;

}

#endif /*OBJECTIVE_H_*/
