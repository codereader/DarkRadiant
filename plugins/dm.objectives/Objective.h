#ifndef OBJECTIVE_H_
#define OBJECTIVE_H_

#include "Component.h"

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

	// The difficulty level this objective applies to
	// A negative value means that this applies to all levels
	int difficultyLevel;

	// The prerequisites of this objective (a space-delimited index list for now)
	std::string enablingObjs;
	
	// Map of indexed components
	typedef std::map<int, Component> ComponentMap;
	ComponentMap components;
	
	// Constructor
	Objective()	: 
		state(INCOMPLETE), 
		mandatory(false), 
		visible(false), 
		ongoing(false),
		irreversible(false),
		difficultyLevel(-1)
	{}
};

/**
 * Objective map type.
 */
typedef std::map<int, Objective> ObjectiveMap;

}

#endif /*OBJECTIVE_H_*/
