#include "FilterUserInterface.h"

#include "ifilter.h"
#include "ieventmanager.h"
#include <sigc++/functors/mem_fun.h>

#include "module/StaticModule.h"

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
		_dependencies.insert(MODULE_EVENTMANAGER);
	}

	return _dependencies;
}

void FilterUserInterface::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	// Create a Toggle item such that menu items can bind to it
	GlobalFilterSystem().forEachFilter([&](const std::string& name)
	{
		auto eventName = GlobalFilterSystem().getFilterEventName(name);
		auto toggleEvent = GlobalEventManager().addToggle(
			eventName,
			std::bind(&FilterUserInterface::toggleFilter, this, std::string(name), std::placeholders::_1)
		);

		_toggleFilterEvents.emplace(name, toggleEvent);

		toggleEvent->setToggled(GlobalFilterSystem().getFilterState(name));
	});

	GlobalEventManager().addCommand("ActivateAllFilters", "ActivateAllFilters");
	GlobalEventManager().addCommand("DeactivateAllFilters", "DeactivateAllFilters");

	_filtersChangedConn = GlobalFilterSystem().filtersChangedSignal().connect(
		sigc::mem_fun(*this, &FilterUserInterface::onFiltersChanged)
	);
}

void FilterUserInterface::shutdownModule()
{
}

void FilterUserInterface::onFiltersChanged()
{
	for (const auto& pair : _toggleFilterEvents)
	{
		pair.second->setToggled(GlobalFilterSystem().getFilterState(pair.first));
	}
}

void FilterUserInterface::toggleFilter(const std::string& filterName, bool newState)
{
	GlobalFilterSystem().setFilterState(filterName, newState);
}

module::StaticModule<FilterUserInterface> filterUserInterfaceModule;

}
