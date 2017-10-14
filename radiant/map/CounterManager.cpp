#include "CounterManager.h"

#include "i18n.h"
#include "iuimanager.h"
#include "selectionlib.h"
#include "string/string.h"
#include "modulesystem/StaticModule.h"

#include <fmt/format.h>

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
	// Don't immediately update the counter text, this is low priority stuff
	requestIdleCallback();
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
		_dependencies.insert(MODULE_SELECTIONSYSTEM);
	}

	return _dependencies;
}

void CounterManager::initialiseModule(const ApplicationContext& ctx)
{
	// Add the statusbar command text item
	GlobalUIManager().getStatusBarManager().addTextElement(
		"MapCounters",
		"",  // no icon
		IStatusBarManager::POS_BRUSHCOUNT,
		_("Number of brushes/patches/entities in this map\n(Number of selected items shown in parentheses)")
	);

	_selectionChangedConn = GlobalSelectionSystem().signal_selectionChanged().connect(
		[this] (const ISelectable&) { requestIdleCallback(); }
	);
}

void CounterManager::shutdownModule()
{
	_selectionChangedConn.disconnect();
}

void CounterManager::onIdle()
{
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	std::string text =
		fmt::format(_("Brushes: {0:d} ({1:d}) Patches: {2:d} ({3:d}) Entities: {4:d} ({5:d})"), 
		_counters[counterBrushes]->get(),
		info.brushCount,
		_counters[counterPatches]->get(),
		info.patchCount,
		_counters[counterEntities]->get(),
		info.entityCount);

	GlobalUIManager().getStatusBarManager().setText("MapCounters", text);
}

// Register the counter module in the registry
module::StaticModule<CounterManager> counterManagerModule;

} // namespace map
