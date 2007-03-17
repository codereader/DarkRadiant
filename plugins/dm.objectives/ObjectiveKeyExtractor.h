#ifndef OBJECTIVEKEYEXTRACTOR_H_
#define OBJECTIVEKEYEXTRACTOR_H_

#include "Objective.h"

#include "gtkutil/image.h"

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <map>

namespace objectives
{

/* CONSTANTS */
namespace {
	
	const char* OBJ_ICON = "objectives16.png";
	
}

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
	void visit(const std::string& key, const std::string& value) {
		
		// Quick discard of any non-objective keys
		if (key.substr(0, 3) != "obj")
			return;
			
		// Extract the objective number
		static const boost::regex reObjNum("obj(\\d+)_(.*)");
		boost::smatch results;
		int iNum;
		
		if (boost::regex_match(key, results, reObjNum)) {
			// Get the objective number
			iNum = boost::lexical_cast<int>(results[1]);			
		}
		else {
			// No match, abort
			return;
		}

		// We now have the objective number and the substring (everything after
		// "obj<n>_" which applies to this objective.
		std::string objSubString = results[2];
		
		// Switch on the substring
		if (objSubString == "desc") {
			_objMap[iNum].description = value;			
		}
	}
	
};

}

#endif /*OBJECTIVEKEYEXTRACTOR_H_*/
