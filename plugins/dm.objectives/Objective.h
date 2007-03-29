#ifndef OBJECTIVE_H_
#define OBJECTIVE_H_

#include <string>
#include <map>

namespace objectives
{

/**
 * Data object representing a single mission objective.
 */
struct Objective
{
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
	
	// Constructor
	Objective()
	: state(INCOMPLETE), mandatory(false), visible(false), ongoing(false),
	  irreversible(false)
	{ }
};

/**
 * Objective map type.
 */
typedef std::map<int, Objective> ObjectiveMap;

}

#endif /*OBJECTIVE_H_*/
