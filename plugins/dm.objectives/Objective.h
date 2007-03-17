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
};

/**
 * Objective map type.
 */
typedef std::map<int, Objective> ObjectiveMap;

}

#endif /*OBJECTIVE_H_*/
