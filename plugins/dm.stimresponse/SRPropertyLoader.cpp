#include "SRPropertyLoader.h"

#include "iregistry.h"
#include "entitylib.h"
#include "string/string.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/regex.hpp>

	namespace {
		const std::string RKEY_STIM_RESPONSE_PREFIX = 
				"game/stimResponseSystem/stimResponsePrefix";
	}

// Constructor
SRPropertyLoader::SRPropertyLoader(
	SREntity::KeyList& keys, 
	SREntity::StimResponseMap& srMap,
	std::string& warnings
) :
	_keys(keys),
	_srMap(srMap),
	_warnings(warnings)
{}

void SRPropertyLoader::visit(const std::string& key, const std::string& value) {
	parseAttribute(key, value, false);
}

void SRPropertyLoader::visit(const EntityClassAttribute& attribute) {
	parseAttribute(attribute.name, attribute.value, true);
}

/** greebo: Private helper method that does the attribute analysis
 */
void SRPropertyLoader::parseAttribute(
	const std::string& key, 
	const std::string& value,
	bool inherited) 
{
	std::string prefix = GlobalRegistry().get(RKEY_STIM_RESPONSE_PREFIX);
	
	// Now cycle through the possible key names and see if we have a match
	for (unsigned int i = 0; i < _keys.size(); i++) {

		// Construct a regex with the number as match variable
		std::string exprStr = "^" + prefix + _keys[i] + "_([0-9])+$";
		boost::regex expr(exprStr);
		boost::smatch matches;
		
		if (boost::regex_match(key, matches, expr)) {
			// Retrieve the S/R index number
			int index = strToInt(matches[1]);
			
			// Check if the S/R with this index already exists
			SREntity::StimResponseMap::iterator found = _srMap.find(index);
			
			if (found == _srMap.end()) {
				// Insert a new SR object with the given index
				_srMap[index] = StimResponse();
				_srMap[index].setIndex(index);
				_srMap[index].setInherited(inherited);
			}
			
			// Check if the property already exists
			if (!_srMap[index].get(_keys[i]).empty()) {
				// already existing, add to the warnings
				_warnings += "Warning on StimResponse #" + intToStr(index) + 
							 ": property " + _keys[i] + " defined more than once.\n";
			}
			
			// Set the property value on the StimResponse object
			_srMap[index].set(_keys[i], value);
		}
	}
}
