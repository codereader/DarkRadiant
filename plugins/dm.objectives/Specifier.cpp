#include "Specifier.h"

#include <stdexcept>

namespace objectives
{

// Static enum count
int Specifier::enumCount = 0;

// Specifier types

const Specifier& Specifier::SPEC_NONE() { 
	static Specifier _instance("none", "No specifier");
	return _instance;
}
const Specifier& Specifier::SPEC_NAME() {
	static Specifier _instance("name", "Name of single entity");
	return _instance;
}
const Specifier& Specifier::SPEC_OVERALL() {
	static Specifier _instance("overall", "Overall (component-specific)");
	return _instance;
}
const Specifier& Specifier::SPEC_GROUP() {
	static Specifier _instance(
		"group", "Group identifier (component-specific)"
	);
	return _instance;
}
const Specifier& Specifier::SPEC_CLASSNAME() {
	static Specifier _instance("classname", "Any entity of specified class");
	return _instance;
}
const Specifier& Specifier::SPEC_SPAWNCLASS() {
	static Specifier _instance(
		"spawnclass", "Any entity with SDK-level spawnclass"
	);
	return _instance;
}
const Specifier& Specifier::SPEC_AI_TYPE() {
	static Specifier _instance("ai_type", "Any AI of specified type");
	return _instance;
}
const Specifier& Specifier::SPEC_AI_TEAM() {
	static Specifier _instance("ai_team", "Any AI on specified team");
	return _instance;
}
const Specifier& Specifier::SPEC_AI_INNOCENCE() {
	static Specifier _instance(
		"ai_innocence", "Any AI with specified combat status"
	);
	return _instance;
}

// Specifier sets

const SpecifierSet& Specifier::SET_ALL() {
	static SpecifierSet _instance;
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
const SpecifierSet& Specifier::SET_STANDARD_AI() {
	static SpecifierSet _instance;
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
Specifier::SpecifierMap& Specifier::getMap() {
	static SpecifierMap _instance;
	return _instance;
}

// Map lookup function
const Specifier& Specifier::getSpecifier(const std::string& name) {
	SpecifierMap::const_iterator i = getMap().find(name);
	if (i != getMap().end())
		return i->second;
	else
		throw std::runtime_error("Specifier " + name + " not found.");
}

// Construct a named Specifier object, incrementing the count
Specifier::Specifier(const std::string& name, const std::string& displayName)
: _id(enumCount++),
  _name(name),
  _displayName(displayName)
{ 
	// Register self in map
	getMap().insert(SpecifierMap::value_type(name, *this));
}

}
