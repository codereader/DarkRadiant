#include "Specifier.h"

#include <stdexcept>

namespace objectives
{

// Static enum count
int Specifier::enumCount = 0;

// Static instances
const Specifier Specifier::SPEC_NONE("none");
const Specifier Specifier::SPEC_NAME("name");
const Specifier Specifier::SPEC_OVERALL("overall");
const Specifier Specifier::SPEC_GROUP("group");
const Specifier Specifier::SPEC_CLASSNAME("classname");
const Specifier Specifier::SPEC_SPAWNCLASS("spawnclass");
const Specifier Specifier::SPEC_AI_TYPE("ai_type");
const Specifier Specifier::SPEC_AI_TEAM("ai_team");
const Specifier Specifier::SPEC_AI_INNOCENCE("ai_innocence");

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
Specifier::Specifier(const std::string& name)
: _id(enumCount++),
  _name(name)
{ 
	// Register self in map
	getMap().insert(SpecifierMap::value_type(name, *this));
}

}
