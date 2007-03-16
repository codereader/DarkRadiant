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
	SREntity::SRList& srList
) :
	_keys(keys),
	_srList(srList)
{}

void SRPropertyLoader::visit(const std::string& key, const std::string& value) {
	parseAttribute(key, value);
}

void SRPropertyLoader::visit(const EntityClassAttribute& attribute) {
	parseAttribute(attribute.name, attribute.value);
}

/** greebo: Private helper method that does the attribute analysis
 */
void SRPropertyLoader::parseAttribute(
	const std::string& key, 
	const std::string& value) 
{
	std::string prefix = GlobalRegistry().get(RKEY_STIM_RESPONSE_PREFIX);
	
	// Now cycle through the possible key names and see if we have a match
	for (unsigned int i = 0; i < _keys.size(); i++) {

		// Construct a regex with the number as match variable
		std::string exprStr = "^" + prefix + _keys[i] + "_([0-9])+$";
		boost::regex expr(exprStr);
		boost::smatch matches;
		
		if (boost::regex_match(key, matches, expr)) {
			// Retrieve the number
			int id = strToInt(matches[1]);
			
			// Insertion code here
		}
	}
}
