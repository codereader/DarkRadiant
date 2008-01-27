#include "ComponentEditorFactory.h"

#include <stdexcept>

namespace objectives
{

namespace ce
{

// Static map instance owner
ComponentEditorMap& ComponentEditorFactory::getMap()
{
	static ComponentEditorMap _instance;
	return _instance;
}

// Create a named ComponentEditor type
ComponentEditorPtr ComponentEditorFactory::create(const std::string& type,
												  objectives::Component& comp)
{
	ComponentEditorMap::const_iterator i = getMap().find(type);
	if (i != getMap().end())
		return i->second->clone(comp);
	else
		return ComponentEditorPtr();
}

// Register a new type
void ComponentEditorFactory::registerType(const std::string& type,
										  ComponentEditorPtr subclass)
{
	getMap().insert(ComponentEditorMap::value_type(type, subclass));
}

}

}
