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
 * and populates the given GtkTreeStore with appropriate information.
 */
class ObjectiveKeyExtractor
: public Entity::Visitor
{
	// Map of number->Objective objects
	typedef std::map<std::string, Objective> ObjectiveMap;
	ObjectiveMap _objMap;

public:

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
		std::string sNum;
		
		if (boost::regex_match(key, results, reObjNum)) {
			// Get the objective number
			sNum = results[1];			
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
			_objMap[sNum].description = value;			
		}
	}
	
	/**
	 * Populate the given list store with values from the map, which has been
	 * built by visiting the entity.
	 */
	void populateList(GtkListStore* store) {
		for (ObjectiveMap::const_iterator i = _objMap.begin();
			 i != _objMap.end();
			 ++i)
		{
			GtkTreeIter iter;
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter,
							   0, i->first.c_str(), // objective number
							   1, i->second.description.c_str(), // description
							   -1);	
		}		
	}
};

}

#endif /*OBJECTIVEKEYEXTRACTOR_H_*/
