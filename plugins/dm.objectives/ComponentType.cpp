#include "ComponentType.h"
#include "util/ObjectivesException.h"
#include "i18n.h"

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
	static ComponentType _instance("kill", _("AI is killed"));
	return _instance;
}

const ComponentType& ComponentType::COMP_KO() {
	static ComponentType _instance("ko", _("AI is knocked out"));
	return _instance;
}

const ComponentType& ComponentType::COMP_AI_FIND_ITEM() {
	static ComponentType _instance("ai_find_item", _("AI finds an item"));
	return _instance;
}

const ComponentType& ComponentType::COMP_AI_FIND_BODY() {
	static ComponentType _instance("ai_find_body", _("AI finds a body"));
	return _instance;
}

const ComponentType& ComponentType::COMP_ALERT() {
	static ComponentType _instance("alert", _("AI is alerted")); // sic
	return _instance;
}

const ComponentType& ComponentType::COMP_DESTROY() {
	static ComponentType _instance("destroy", _("Object is destroyed"));
	return _instance;
}

const ComponentType& ComponentType::COMP_ITEM() {
	static ComponentType _instance("item", _("Player possesses item"));
	return _instance;
}

const ComponentType& ComponentType::COMP_PICKPOCKET() {
	static ComponentType _instance("pickpocket", _("Player pickpockets AI"));
	return _instance;
}

const ComponentType& ComponentType::COMP_LOCATION() {
	static ComponentType _instance("location", _("Item is in location"));
	return _instance;
}

const ComponentType& ComponentType::COMP_INFO_LOCATION() {
	static ComponentType _instance("info_location", _("Item is in info_location"));
	return _instance;
}

const ComponentType& ComponentType::COMP_CUSTOM_ASYNC() {
	static ComponentType _instance("custom", _("Custom script")); // sic
	return _instance;
}

const ComponentType& ComponentType::COMP_CUSTOM_CLOCKED() {
	static ComponentType _instance(
		"custom_clocked", _("Custom script queried periodically")
	);
	return _instance;
}

const ComponentType& ComponentType::COMP_DISTANCE() {
	static ComponentType _instance(
		"distance", _("Two entities are within a radius of each other")
	);
	return _instance;
}

const ComponentType& ComponentType::COMP_READABLE_OPENED()
{
	static ComponentType _instance(
		"readable_opened", _("Readable is opened.")
	);
	return _instance;
}

const ComponentType& ComponentType::COMP_READABLE_CLOSED()
{
	static ComponentType _instance(
		"readable_closed", _("Readable is closed.")
	);
	return _instance;
}

const ComponentType& ComponentType::COMP_READABLE_PAGE_REACHED()
{
	static ComponentType _instance(
		"readable_page_reached", _("A certain page of a readable is reached.")
	);
	return _instance;
}

// Static sets
const ComponentTypeSet& ComponentType::SET_ALL() {
	static ComponentTypeSet _instance;
	if (_instance.empty()) {
		_instance.insert(COMP_KILL());
		_instance.insert(COMP_KO());
		_instance.insert(COMP_AI_FIND_ITEM());
		_instance.insert(COMP_AI_FIND_BODY());
		_instance.insert(COMP_ALERT());
		_instance.insert(COMP_DESTROY());
		_instance.insert(COMP_ITEM());
		_instance.insert(COMP_PICKPOCKET());
		_instance.insert(COMP_LOCATION());
		_instance.insert(COMP_INFO_LOCATION());
		_instance.insert(COMP_CUSTOM_ASYNC());
		_instance.insert(COMP_CUSTOM_CLOCKED());
		_instance.insert(COMP_DISTANCE());
		_instance.insert(COMP_READABLE_OPENED());
		_instance.insert(COMP_READABLE_CLOSED());
		_instance.insert(COMP_READABLE_PAGE_REACHED());
	}
	return _instance;
}

// Construct a named ComponentType
ComponentType::ComponentType(const std::string& name,
							 const std::string& displayName)
: _id(enumCount++),
  _name(name),
  _displayName(displayName)
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
		throw ObjectivesException("Invalid ComponentType: " + name);
}

}
