#include "CounterManager.h"

#include "i18n.h"
#include "iuimanager.h"
#include "string/string.h"
#include "modulesystem/StaticModule.h"

#include <boost/format.hpp>

namespace map
{

CounterManager::CounterManager()
{
	// Create the counter objects
	_counters[counterBrushes] = CounterPtr(new Counter(this));
	_counters[counterPatches] = CounterPtr(new Counter(this));
	_counters[counterEntities] = CounterPtr(new Counter(this));
}

ICounter& CounterManager::getCounter(CounterType counter)
{
	if (_counters.find(counter) == _counters.end()) {
		throw std::runtime_error("Counter ID not found.");
	}
	return *_counters[counter];
}

void CounterManager::countChanged()
{
	std::string text =
		(boost::format(_("Brushes: %lu Patches: %lu Entities: %lu")) %
		_counters[counterBrushes]->get() %
		_counters[counterPatches]->get() %
		_counters[counterEntities]->get()).str();

	GlobalUIManager().getStatusBarManager().setText("MapCounters", text);
}

// RegisterableModule implementation
const std::string& CounterManager::getName() const
{
	static std::string _name(MODULE_COUNTER);
	return _name;
}

const StringSet& CounterManager::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_UIMANAGER);
	}

	return _dependencies;
}

void CounterManager::initialiseModule(const ApplicationContext& ctx)
{
	// Add the statusbar command text item
	GlobalUIManager().getStatusBarManager().addTextElement(
		"MapCounters",
		"",  // no icon
		IStatusBarManager::POS_BRUSHCOUNT
	);
}

// Register the counter module in the registry
module::StaticModule<CounterManager> counterManagerModule;

} // namespace map
