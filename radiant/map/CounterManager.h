#pragma once

#include "icounter.h"
#include "iuimanager.h"
#include "wxutil/event/SingleIdleCallback.h"

#include <sigc++/connection.h>
#include <map>
#include <memory>

namespace map {

class Counter :
	public ICounter
{
	Observer* _observer;
	std::size_t _count;
public:
	Counter(Observer* observer = NULL) :
		_observer(observer),
		_count(0)
	{}

	virtual ~Counter() {}

	void increment() {
		++_count;

		if (_observer != NULL) {
			_observer->countChanged();
		}
	}

	void decrement() {
		--_count;

		if (_observer != NULL) {
			_observer->countChanged();
		}
	}

	std::size_t get() const {
		return _count;
	}
};
typedef std::shared_ptr<Counter> CounterPtr;

class CounterManager :
	public ICounterManager,
	public ICounter::Observer,
	protected wxutil::SingleIdleCallback
{
	typedef std::map<CounterType, CounterPtr> CounterMap;
	CounterMap _counters;

	sigc::connection _selectionChangedConn;

public:
	CounterManager();

	virtual ~CounterManager() {}

	ICounter& getCounter(CounterType counter) override;

	// ICounter::Observer implementation
	void countChanged() override;

	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const ApplicationContext& ctx) override;
	void shutdownModule() override;

protected:
	void onIdle() override;
};

} // namespace map
