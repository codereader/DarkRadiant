#include "SpecifierType.h"
#include "util/ObjectivesException.h"

#include "i18n.h"

namespace objectives
{

// Static enum count
int SpecifierType::enumCount = 0;

// SpecifierType types

const SpecifierType& SpecifierType::SPEC_NONE() {
	static SpecifierType _instance("none", _("No specifier"));
	return _instance;
}
const SpecifierType& SpecifierType::SPEC_NAME() {
	static SpecifierType _instance("name", _("Name of single entity"));
	return _instance;
}
const SpecifierType& SpecifierType::SPEC_OVERALL() {
	static SpecifierType _instance("overall", _("Overall (component-specific)"));
	return _instance;
}
const SpecifierType& SpecifierType::SPEC_GROUP() {
	static SpecifierType _instance(
		"group", _("Group identifier (component-specific)")
	);
	return _instance;
}
const SpecifierType& SpecifierType::SPEC_CLASSNAME() {
	static SpecifierType _instance("classname", _("Any entity of specified class"));
	return _instance;
}
const SpecifierType& SpecifierType::SPEC_SPAWNCLASS() {
	static SpecifierType _instance(
		"spawnclass", _("Any entity with SDK-level spawnclass")
	);
	return _instance;
}
const SpecifierType& SpecifierType::SPEC_AI_TYPE() {
	static SpecifierType _instance("ai_type", _("Any AI of specified type"));
	return _instance;
}
const SpecifierType& SpecifierType::SPEC_AI_TEAM() {
	static SpecifierType _instance("ai_team", _("Any AI on specified team"));
	return _instance;
}
const SpecifierType& SpecifierType::SPEC_AI_INNOCENCE() {
	static SpecifierType _instance(
		"ai_innocence", _("Any AI with specified combat status")
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

const SpecifierTypeSet& SpecifierType::SET_READABLE()
{
	static SpecifierTypeSet _instance;

	if (_instance.empty())
	{
		_instance.insert(SPEC_NONE());
		_instance.insert(SPEC_NAME());
	}

	return _instance;
}

const SpecifierTypeSet& SpecifierType::SET_LOCATION() {
	static SpecifierTypeSet _instance;

	if (_instance.empty()) {
		_instance.insert(SPEC_NONE());
		_instance.insert(SPEC_NAME());
		_instance.insert(SPEC_GROUP());
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
SpecifierType::SpecifierTypeMap& SpecifierType::getMap()
{
	static SpecifierTypeMap _instance;

	if (_instance.empty())
	{
		// greebo: Make sure all default specifiers are in the map
		_instance.insert(SpecifierTypeMap::value_type(SPEC_NONE().getName(), SPEC_NONE()));
		_instance.insert(SpecifierTypeMap::value_type(SPEC_NAME().getName(), SPEC_NAME()));
		_instance.insert(SpecifierTypeMap::value_type(SPEC_OVERALL().getName(), SPEC_OVERALL()));
		_instance.insert(SpecifierTypeMap::value_type(SPEC_GROUP().getName(), SPEC_GROUP()));
		_instance.insert(SpecifierTypeMap::value_type(SPEC_CLASSNAME().getName(), SPEC_CLASSNAME()));
		_instance.insert(SpecifierTypeMap::value_type(SPEC_SPAWNCLASS().getName(), SPEC_SPAWNCLASS()));
		_instance.insert(SpecifierTypeMap::value_type(SPEC_AI_TYPE().getName(), SPEC_AI_TYPE()));
		_instance.insert(SpecifierTypeMap::value_type(SPEC_AI_TEAM().getName(), SPEC_AI_TEAM()));
		_instance.insert(SpecifierTypeMap::value_type(SPEC_AI_INNOCENCE().getName(), SPEC_AI_INNOCENCE()));
	}

	return _instance;
}

// Map lookup function
const SpecifierType& SpecifierType::getSpecifierType(const std::string& name)
{
	std::string specName = name.empty() ? "none" : name;

	SpecifierTypeMap::const_iterator i = getMap().find(specName);

	if (i != getMap().end())
	{
		return i->second;
	}
	else
	{
		throw ObjectivesException("SpecifierType " + name + " not found.");
	}
}

// Construct a named SpecifierType object, incrementing the count
SpecifierType::SpecifierType(const std::string& name, const std::string& displayName)
: _id(enumCount++),
  _name(name),
  _displayName(displayName)
{ }

}
