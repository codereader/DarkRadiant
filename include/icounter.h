#ifndef ICOUNTER_H_
#define ICOUNTER_H_

#include <cstddef>
#include "imodule.h"

class ICounter {
public:
	class Observer
	{
	public:
		virtual ~Observer() {}
		// Gets called by the Counter class on count change
		virtual void countChanged() = 0;
	};

    /** Destructor */
	virtual ~ICounter() {}

	/** greebo: Decrements/increments the counter.
	 */
	virtual void increment() = 0;
	virtual void decrement() = 0;

	/** greebo: Returns the current count.
	 */
	virtual std::size_t get() const = 0;
};

// Known counters
enum CounterType
{
	counterBrushes,
	counterPatches,
	counterEntities,
};

const std::string MODULE_COUNTER("Counters");

/** greebo: This abstract class defines the interface to the core application.
 * 			Use this to access methods from the main codebase in radiant/
 */
class ICounterManager :
	public RegisterableModule
{
public:
	// Returns the Counter object of the given type
	virtual ICounter& getCounter(CounterType counter) = 0;
};

inline ICounterManager& GlobalCounters()
{
	// Cache the reference locally
	static ICounterManager& _counters(
		*boost::static_pointer_cast<ICounterManager>(
			module::GlobalModuleRegistry().getModule(MODULE_COUNTER)
		)
	);
	return _counters;
}

#endif /*ICOUNTER_H_*/
