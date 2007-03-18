#ifndef OBJECTIVEKEYEXTRACTOR_H_
#define OBJECTIVEKEYEXTRACTOR_H_

#include "Objective.h"

#include "ientity.h"

namespace objectives
{

/**
 * Entity Visitor which extracts objective keyvalues (of the form "obj<n>_blah")
 * and populates the given ObjectiveMap with the parsed objective objects.
 */
class ObjectiveKeyExtractor
: public Entity::Visitor
{
	// Map of number->Objective objects
	ObjectiveMap& _objMap;

public:

	/**
	 * Constructor. Sets the map to populate.
	 */
	ObjectiveKeyExtractor(ObjectiveMap& map)
	: _objMap(map)
	{ 
		assert(_objMap.empty());
	}

	/**
	 * Required visit function.
	 */
	void visit(const std::string& key, const std::string& value);
	
};

}

#endif /*OBJECTIVEKEYEXTRACTOR_H_*/
