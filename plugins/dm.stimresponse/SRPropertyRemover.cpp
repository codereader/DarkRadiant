#include "SRPropertyRemover.h"

#include "iregistry.h"
#include "entitylib.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/regex.hpp>

SRPropertyRemover::SRPropertyRemover(
	Entity* target, 
	SREntity::KeyList& keys
) :
	_target(target),
	_keys(keys)
{}

SRPropertyRemover::~SRPropertyRemover() {
	for (unsigned int i = 0; i < _removeList.size(); i++) {
		// Delete the key
		_target->setKeyValue(_removeList[i], "");
	}
}

void SRPropertyRemover::visit(const std::string& key, const std::string& value) {
	std::string prefix = GlobalRegistry().get(RKEY_STIM_RESPONSE_PREFIX);
	
	// Now cycle through the possible key names and see if we have a match
	for (unsigned int i = 0; i < _keys.size(); i++) {
		// Construct a regex with the number as match variable
		std::string exprStr = "^" + prefix + _keys[i].key + "_([0-9])+$";
		boost::regex expr(exprStr);
		boost::smatch matches;
		
		if (boost::regex_match(key, matches, expr)) {
			// We have a match, set the key on the black list
			_removeList.push_back(key);
		}
	}
	
	// This should search for something like "sr_effect_2_3*" 
	std::string exprStr = "^" + prefix + "effect" + "_([0-9])+_([0-9])+(.*)$";
	boost::regex expr(exprStr);
	boost::smatch matches;
	
	if (boost::regex_match(key, matches, expr)) {
		_removeList.push_back(key);
	}
}
