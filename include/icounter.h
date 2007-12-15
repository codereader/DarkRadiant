#ifndef ICOUNTER_H_
#define ICOUNTER_H_

#include <cstddef>

class ICounter {
public:
	class Observer
	{
	public:
		// Gets called by the Counter class on count change
		virtual void countChanged() = 0;
	};

	/** greebo: Decrements/increments the counter.
	 */
	virtual void increment() = 0;
	virtual void decrement() = 0;
	
	/** greebo: Returns the current count.
	 */
	virtual std::size_t get() const = 0;
};

#endif /*ICOUNTER_H_*/
