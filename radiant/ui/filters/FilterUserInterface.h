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
	std::map<std::string, IEventPtr> _toggleFilterEvents;

	sigc::connection _filtersChangedConn;

public:
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const ApplicationContext& ctx) override;
	void shutdownModule() override;

private:
	void onFiltersChanged();
	void toggleFilter(const std::string& filterName, bool newState);
};

}
