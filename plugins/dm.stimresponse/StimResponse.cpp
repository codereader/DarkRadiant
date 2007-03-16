#include "StimResponse.h"

StimResponse::StimResponse() :
	_class(typeStim),
	_inherited(false)
{}

// Copy constructor
StimResponse::StimResponse(const StimResponse& other) :
	_class(other._class),
	_inherited(other._inherited),
	_properties(other._properties)
{}

/** greebo: Gets the property value string or "" if not defined/empty
 */
std::string StimResponse::get(const std::string& key) {
	
	PropertyMap::iterator i = _properties.find(key);
	
	if (i != _properties.end()) {
		return _properties[key];
	}
	else {
		// Not found, return an empty string
		return "";
	}
}

void StimResponse::set(const std::string& key, const std::string& value) {
	_properties[key] = value;
}

void StimResponse::setInherited(bool inherited) {
	_inherited = inherited;
}

bool StimResponse::inherited() const {
	return _inherited;
}
