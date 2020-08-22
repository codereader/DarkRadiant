#pragma once

#include "icounter.h"

#include <sigc++/connection.h>
#include <map>
#include <memory>

namespace map
{

class CounterManager;

class Counter :
	public ICounter
{
private:
	CounterManager& _owner;
	std::size_t _count;

public:
	Counter(CounterManager& owner);

	virtual ~Counter() {}

	void increment() override;
	void decrement() override;
	
	std::size_t get() const override;
};
typedef std::shared_ptr<Counter> CounterPtr;

class CounterManager :
	public ICounterManager
{
private:
	typedef std::map<CounterType, CounterPtr> CounterMap;
	CounterMap _counters;

	sigc::signal<void> _signalCountersChanged;

public:
	CounterManager();

	virtual ~CounterManager() {}

	ICounter& getCounter(CounterType counter) override;

	sigc::signal<void>& signal_countersChanged() override;

	void onCounterChanged();

	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;
};

} // namespace map
