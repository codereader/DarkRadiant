#include "CounterManager.h"

#include "module/StaticModule.h"

namespace map
{

Counter::Counter(CounterManager& owner) :
	_owner(owner),
	_count(0)
{}

void Counter::increment()
{
	++_count;

	_owner.onCounterChanged();
}

void Counter::decrement()
{
	--_count;

	_owner.onCounterChanged();
}

std::size_t Counter::get() const
{
	return _count;
}

CounterManager::CounterManager()
{
	// Create the counter objects
	_counters[counterBrushes] =  std::make_shared<Counter>(*this);
	_counters[counterPatches] =  std::make_shared<Counter>(*this);
	_counters[counterEntities] = std::make_shared<Counter>(*this);
}

ICounter& CounterManager::getCounter(CounterType counter)
{
	if (_counters.find(counter) == _counters.end())
	{
		throw std::runtime_error("Counter ID not found.");
	}

	return *_counters[counter];
}

sigc::signal<void>& CounterManager::signal_countersChanged()
{
	return _signalCountersChanged;
}

void CounterManager::onCounterChanged()
{
	_signalCountersChanged.emit();
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
	return _dependencies;
}

void CounterManager::initialiseModule(const IApplicationContext& ctx)
{
}

// Register the counter module in the registry
module::StaticModuleRegistration<CounterManager> counterManagerModule;

} // namespace map
