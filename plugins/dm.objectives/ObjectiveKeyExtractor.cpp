#include "ObjectiveKeyExtractor.h"

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string/case_conv.hpp>

namespace objectives {
	
// Required entity visit function
void ObjectiveKeyExtractor::visit(const std::string& key, 
								  const std::string& value)
{
	using boost::lexical_cast;
	
	// Quick discard of any non-objective keys
	if (key.substr(0, 3) != "obj")
		return;
		
	// Extract the objective number
	static const boost::regex reObjNum("obj(\\d+)_(.*)");
	boost::smatch results;
	int iNum;
	
	if (boost::regex_match(key, results, reObjNum)) {
		// Get the objective number
		iNum = lexical_cast<int>(results[1]);			
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
	else if (objSubString == "state") {
		_objMap[iNum].state = 
			static_cast<Objective::State>(lexical_cast<int>(value));
	}
	else {
	
		// Use another regex to check for components (obj1_1_blah)
		static const boost::regex reComponent("(\\d+)_(.*)");
		boost::smatch results;
		
		if (!boost::regex_match(objSubString, results, reComponent)) {
			return;
		}
		else {
			
			// Get the component number and key string
			int componentNum = boost::lexical_cast<int>(results[1]);
			std::string componentStr = results[2];
			
			Component& comp = _objMap[iNum].components[componentNum];
			
			// Switch on the key string
			if (componentStr == "type") {
				comp.type = boost::algorithm::to_upper_copy(value); 
			}
		}
			
		
	}

}
	
} // namespace objectives
