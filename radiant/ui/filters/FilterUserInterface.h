#pragma once

#include <map>
#include <sigc++/connection.h>
#include "imodule.h"

namespace ui
{

class FilterUserInterface :
	public RegisterableModule
{
private:
	typedef std::pair<std::string, IEventPtr> NamedToggle;
	std::map<std::string, NamedToggle> _toggleFilterEvents;

	sigc::connection _filterConfigChangedConn;
	sigc::connection _filterCollectionChangedConn;

public:
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const ApplicationContext& ctx) override;
	void shutdownModule() override;

private:
	void onFilterConfigChanged();
	void onFilterCollectionChanged();
	void refreshFilterToggles();
	void toggleFilter(const std::string& filterName, bool newState);
};

}
