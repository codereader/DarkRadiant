#pragma once

#include <boost/shared_ptr.hpp>

namespace objectives
{

/**
 * greebo: An objective condition is providing a way to change
 * a single objective in this mission based on the status of one
 * in a previous mission. 
 */
class ObjectiveCondition
{
public:
	// The index of the source mission (0-based)
	int sourceMission;

	// The index of the source objective (0-based)
	int sourceObjective; 

	// The index of the target objective
	int targetObjective;


};
typedef boost::shared_ptr<ObjectiveCondition> ObjectiveConditionPtr;

} // namespace
