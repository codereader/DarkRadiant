#pragma once

#include <boost/shared_ptr.hpp>
#include "Objective.h"

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

	// Source objective state
	Objective::State sourceState;

	// The index of the target objective (0-based)
	int targetObjective;

	enum Type
	{
		CHANGE_STATE,		// changes state of target objective
		CHANGE_VISIBILITY,	// changes visibility of target objective
		CHANGE_MANDATORY,	// changes mandatory flag of target objetive
		INVALID_TYPE,		// not a valid type
	};

	Type type;

	// The value, can have different meanings depending on type (-1 == invalid)
	int value;

	// Default constructor, will construct an empty/invalid condition
	ObjectiveCondition() :
		sourceMission(-1),
		sourceObjective(-1),
		sourceState(Objective::NUM_STATES),
		targetObjective(-1),
		type(INVALID_TYPE),
		value(-1)
	{}

	bool isValid() const
	{
		// Some checks to see whether this is an empty or invalid condition
		return type != INVALID_TYPE && sourceMission != -1 && sourceState != Objective::NUM_STATES &&
			   sourceObjective != -1 && targetObjective != -1 && value != -1;
	}
};
typedef boost::shared_ptr<ObjectiveCondition> ObjectiveConditionPtr;

} // namespace
