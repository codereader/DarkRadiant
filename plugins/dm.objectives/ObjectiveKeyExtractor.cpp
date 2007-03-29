#include "ObjectiveKeyExtractor.h"

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

namespace objectives {
	
// Required entity visit function
void ObjectiveKeyExtractor::visit(const std::string& key, 
								  const std::string& value)
{
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
	else if (objSubString == "ongoing") {
		_objMap[iNum].ongoing = (value == "1");			
	}
	else if (objSubString == "mandatory") {
		_objMap[iNum].mandatory = (value == "1");			
	}
	else if (objSubString == "visible") {
		_objMap[iNum].visible = (value == "1");			
	}
	else if (objSubString == "irreversible") {
		_objMap[iNum].irreversible = (value == "1");			
	}

}
	
} // namespace objectives
