#include "SpecifierType.h"
#include "util/ObjectivesException.h"

namespace objectives
{

// Static enum count
int SpecifierType::enumCount = 0;

// SpecifierType types

const SpecifierType& SpecifierType::SPEC_NONE() { 
	static SpecifierType _instance("none", "No specifier");
	return _instance;
}
const SpecifierType& SpecifierType::SPEC_NAME() {
	static SpecifierType _instance("name", "Name of single entity");
	return _instance;
}
const SpecifierType& SpecifierType::SPEC_OVERALL() {
	static SpecifierType _instance("overall", "Overall (component-specific)");
	return _instance;
}
const SpecifierType& SpecifierType::SPEC_GROUP() {
	static SpecifierType _instance(
		"group", "Group identifier (component-specific)"
	);
	return _instance;
}
const SpecifierType& SpecifierType::SPEC_CLASSNAME() {
	static SpecifierType _instance("classname", "Any entity of specified class");
	return _instance;
}
const SpecifierType& SpecifierType::SPEC_SPAWNCLASS() {
	static SpecifierType _instance(
		"spawnclass", "Any entity with SDK-level spawnclass"
	);
	return _instance;
}
const SpecifierType& SpecifierType::SPEC_AI_TYPE() {
	static SpecifierType _instance("ai_type", "Any AI of specified type");
	return _instance;
}
const SpecifierType& SpecifierType::SPEC_AI_TEAM() {
	static SpecifierType _instance("ai_team", "Any AI on specified team");
	return _instance;
}
const SpecifierType& SpecifierType::SPEC_AI_INNOCENCE() {
	static SpecifierType _instance(
		"ai_innocence", "Any AI with specified combat status"
	);
	return _instance;
}

// SpecifierType sets

const SpecifierTypeSet& SpecifierType::SET_ALL() {
	static SpecifierTypeSet _instance;
	if (_instance.empty()) {
		_instance.insert(SPEC_NONE());
		_instance.insert(SPEC_NAME());
		_instance.insert(SPEC_OVERALL());
		_instance.insert(SPEC_GROUP());
		_instance.insert(SPEC_CLASSNAME());
		_instance.insert(SPEC_SPAWNCLASS());
		_instance.insert(SPEC_AI_TYPE());
		_instance.insert(SPEC_AI_TEAM());
		_instance.insert(SPEC_AI_INNOCENCE());
	}
	return _instance;
}

const SpecifierTypeSet& SpecifierType::SET_ITEM() {
	static SpecifierTypeSet _instance;
	if (_instance.empty()) {
		_instance.insert(SPEC_NONE());
		_instance.insert(SPEC_NAME());
		_instance.insert(SPEC_OVERALL());
		_instance.insert(SPEC_GROUP());
		_instance.insert(SPEC_CLASSNAME());
		_instance.insert(SPEC_SPAWNCLASS());
	}
	return _instance;
}

const SpecifierTypeSet& SpecifierType::SET_STANDARD_AI() {
	static SpecifierTypeSet _instance;
	if (_instance.empty()) {
		_instance.insert(SPEC_NONE());
		_instance.insert(SPEC_NAME());
		_instance.insert(SPEC_OVERALL());
		_instance.insert(SPEC_CLASSNAME());
		_instance.insert(SPEC_SPAWNCLASS());
		_instance.insert(SPEC_AI_TYPE());
		_instance.insert(SPEC_AI_TEAM());
		_instance.insert(SPEC_AI_INNOCENCE());
	}
	return _instance;
}

// Map instance owner
SpecifierType::SpecifierTypeMap& SpecifierType::getMap() {
	static SpecifierTypeMap _instance;
	return _instance;
}

// Map lookup function
const SpecifierType& SpecifierType::getSpecifierType(const std::string& name) {
	SpecifierTypeMap::const_iterator i = getMap().find(name);
	if (i != getMap().end())
		return i->second;
	else
		throw ObjectivesException("SpecifierType " + name + " not found.");
}

// Construct a named SpecifierType object, incrementing the count
SpecifierType::SpecifierType(const std::string& name, const std::string& displayName)
: _id(enumCount++),
  _name(name),
  _displayName(displayName)
{ 
	// Register self in map
	getMap().insert(SpecifierTypeMap::value_type(name, *this));
}

}
