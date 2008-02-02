#include "SpecifierPanelFactory.h"

namespace objectives
{

namespace ce
{

// Static instance owner
SpecifierPanelFactory::PanelMap& SpecifierPanelFactory::getMap()
{
	static PanelMap _instance;
	return _instance;	
}

// Register a panel map
void SpecifierPanelFactory::registerType(const std::string& name, 
										 SpecifierPanelPtr cls)
{
	getMap().insert(PanelMap::value_type(name, cls));
}

// Create a panel type
SpecifierPanelPtr SpecifierPanelFactory::create(const std::string& name)
{
	PanelMap::const_iterator i = getMap().find(name);
	if (i != getMap().end())
		return i->second->clone();
	else
		return SpecifierPanelPtr();
}

}

}
