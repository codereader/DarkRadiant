#include "StimResponse.h"

StimResponse::StimResponse() :
	_class(typeStim),
	_inherited(false),
	_index(0)
{}

// Copy constructor
StimResponse::StimResponse(const StimResponse& other) :
	_class(other._class),
	_inherited(other._inherited),
	_properties(other._properties),
	_index(other._index)
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

int StimResponse::getIndex() const {
	return _index;
}

void StimResponse::setIndex(int index) {
	if (!_inherited) {
		_index = index;
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
