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
void SpecifierPanelFactory::registerType(const objectives::Specifier& type, 
										 SpecifierPanelPtr cls)
{
	getMap().insert(PanelMap::value_type(type, cls));
}

// Create a panel type
SpecifierPanelPtr SpecifierPanelFactory::create(objectives::Specifier& type)
{
	PanelMap::const_iterator i = getMap().find(type);
	if (i != getMap().end())
		return i->second->clone();
	else
		return SpecifierPanelPtr();
}

}

}
