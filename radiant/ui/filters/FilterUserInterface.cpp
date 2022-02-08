#include "FilterUserInterface.h"

#include "ifilter.h"
#include "ui/imainframe.h"
#include "ui/imenumanager.h"
#include "icommandsystem.h"
#include <sigc++/functors/mem_fun.h>

#include "module/StaticModule.h"
#include "FiltersMainMenu.h"
#include "editor/FilterDialog.h"

namespace ui
{

const std::string& FilterUserInterface::getName() const
{
	static std::string _name("FilterUserInterface");
	return _name;
}

const StringSet& FilterUserInterface::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_FILTERSYSTEM);
		_dependencies.insert(MODULE_COMMANDSYSTEM);
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_MENUMANAGER);
	}

	return _dependencies;
}

void FilterUserInterface::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	// Create the Toggle objects to connect menu items and toggle buttons
	refreshFilterToggles();

	_filterConfigChangedConn = GlobalFilterSystem().filterConfigChangedSignal().connect(
		sigc::mem_fun(*this, &FilterUserInterface::onFilterConfigChanged)
	);

	_filterCollectionChangedConn = GlobalFilterSystem().filterCollectionChangedSignal().connect(
		sigc::mem_fun(*this, &FilterUserInterface::onFilterCollectionChanged)
	);

	// Create the main menu Filter entries
	FiltersMenu::addItemsToMainMenu();

	// Editor
	GlobalCommandSystem().addCommand("EditFiltersDialog", FilterDialog::ShowDialog);
}

void FilterUserInterface::shutdownModule()
{
	_filterConfigChangedConn.disconnect();
	_filterCollectionChangedConn.disconnect();
}

void FilterUserInterface::onFilterConfigChanged()
{
	for (const auto& pair : _toggleFilterEvents)
	{
		pair.second.second->setToggled(GlobalFilterSystem().getFilterState(pair.first));
	}

	GlobalMainFrame().updateAllWindows();
}

void FilterUserInterface::onFilterCollectionChanged()
{
	refreshFilterToggles();

	// Recreate the main menu Filter entries
	FiltersMenu::removeItemsFromMainMenu();
	FiltersMenu::addItemsToMainMenu();
}

void FilterUserInterface::refreshFilterToggles()
{
	// Remove the old set of events first
	for (const auto& pair : _toggleFilterEvents)
	{
		GlobalEventManager().removeEvent(pair.second.first);
	}

	_toggleFilterEvents.clear();

	// Create a Toggle item such that menu items can bind to it
	GlobalFilterSystem().forEachFilter([&](const std::string& name)
	{
		auto eventName = GlobalFilterSystem().getFilterEventName(name);
		auto toggleEvent = GlobalEventManager().addToggle(
			eventName,
			std::bind(&FilterUserInterface::toggleFilter, this, std::string(name), std::placeholders::_1)
		);

		_toggleFilterEvents.emplace(name, std::make_pair(eventName, toggleEvent));

		toggleEvent->setToggled(GlobalFilterSystem().getFilterState(name));
	});
}

void FilterUserInterface::toggleFilter(const std::string& filterName, bool newState)
{
	GlobalFilterSystem().setFilterState(filterName, newState);
}

module::StaticModuleRegistration<FilterUserInterface> filterUserInterfaceModule;

}
