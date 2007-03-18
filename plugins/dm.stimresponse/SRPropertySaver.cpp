#include "SRPropertySaver.h"

#include "iregistry.h"
#include "entitylib.h"
#include "string/string.h"

SRPropertySaver::SRPropertySaver(Entity* target, SREntity::KeyList& keys) :
	_target(target),
	_keys(keys)
{}

void SRPropertySaver::visit(StimResponse& sr) {
	if (sr.inherited()) {
		// Don't save inherited properties
		return;
	}
	
	std::string prefix = GlobalRegistry().get(RKEY_STIM_RESPONSE_PREFIX);
	std::string suffix = "_" + intToStr(sr.getIndex());
	
	// Now cycle through the possible key names and see if we have a match
	for (unsigned int i = 0; i < _keys.size(); i++) {
		// Check if the property carries a non-empty value
		std::string value = sr.get(_keys[i].key);
		
		// The class of this S/R
		std::string srClass = sr.get("class");
		
		// Check if the property matches for the class type
		if (!value.empty() && _keys[i].classes.find(srClass, 0) != std::string::npos) {
			std::string fullKey = prefix + _keys[i].key + suffix;
			_target->setKeyValue(fullKey, value); 
		}
	}
}
