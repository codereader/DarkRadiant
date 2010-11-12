#ifndef COUNTERMANAGER_H_
#define COUNTERMANAGER_H_

#include "icounter.h"
#include "iuimanager.h"

#include <map>
#include <boost/shared_ptr.hpp>

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
typedef boost::shared_ptr<Counter> CounterPtr;

class CounterManager :
	public ICounterManager,
	public ICounter::Observer
{
	typedef std::map<CounterType, CounterPtr> CounterMap;
	CounterMap _counters;

public:
	CounterManager();

	virtual ~CounterManager() {}

	ICounter& getCounter(CounterType counter);

	// ICounter::Observer implementation
	void countChanged();

	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
};

} // namespace map

#endif /*COUNTERMANAGER_H_*/
