#include "ComponentType.h"

#include <stdexcept>

namespace objectives
{

// Enum count
int ComponentType::enumCount = 0;

// Static map
ComponentType::ComponentTypeMap& ComponentType::getMap() {
	static ComponentTypeMap _instance;
	return _instance;
}

// Static instances

const ComponentType& ComponentType::COMP_KILL() {
	static ComponentType _instance("kill");
	return _instance;
}
const ComponentType& ComponentType::COMP_KO() {
	static ComponentType _instance("ko");
	return _instance;
}
const ComponentType& ComponentType::COMP_AI_FIND_BODY() {
	static ComponentType _instance("ai_find_body");
	return _instance;
}
const ComponentType& ComponentType::COMP_AI_ALERT() {
	static ComponentType _instance("alert"); // sic
	return _instance;
}
const ComponentType& ComponentType::COMP_DESTROY() {
	static ComponentType _instance("destroy");
	return _instance;
}
const ComponentType& ComponentType::COMP_ITEM() {
	static ComponentType _instance("item");
	return _instance;
}
const ComponentType& ComponentType::COMP_PICKPOCKET() {
	static ComponentType _instance("pickpocket");
	return _instance;
}
const ComponentType& ComponentType::COMP_LOCATION() {
	static ComponentType _instance("location");
	return _instance;
}
const ComponentType& ComponentType::COMP_INFO_LOCATION() {
	static ComponentType _instance("info_location");
	return _instance;
}
const ComponentType& ComponentType::COMP_CUSTOM_ASYNC() {
	static ComponentType _instance("custom"); // sic
	return _instance;
}
const ComponentType& ComponentType::COMP_CUSTOM_CLOCKED() {
	static ComponentType _instance("custom_clocked");
	return _instance;
}
const ComponentType& ComponentType::COMP_DISTANCE() {
	static ComponentType _instance("comp_distance"); // sic
	return _instance;
}

// Static sets
const ComponentTypeSet& ComponentType::SET_ALL() {
	static ComponentTypeSet _instance;
	if (_instance.empty()) {
		_instance.insert(COMP_KILL());
		_instance.insert(COMP_KO());
		_instance.insert(COMP_AI_FIND_BODY());
		_instance.insert(COMP_AI_ALERT());
		_instance.insert(COMP_DESTROY());
		_instance.insert(COMP_ITEM());
		_instance.insert(COMP_PICKPOCKET());
		_instance.insert(COMP_LOCATION());
		_instance.insert(COMP_INFO_LOCATION());
		_instance.insert(COMP_CUSTOM_ASYNC());
		_instance.insert(COMP_CUSTOM_CLOCKED());
		_instance.insert(COMP_DISTANCE());
	}
	return _instance;
}

// Construct a named ComponentType
ComponentType::ComponentType(const std::string& name)
: _id(enumCount++),
  _name(name)
{
	// Register self in map
	getMap().insert(ComponentTypeMap::value_type(name, *this));
}

// Lookup a named type in map
ComponentType ComponentType::getComponentType(const std::string& name) 
{
	ComponentTypeMap::const_iterator i = getMap().find(name);
	if (i != getMap().end())
		return i->second;
	else
		throw std::runtime_error("Invalid ComponentType: " + name);
}

}
