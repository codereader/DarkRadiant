#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#include <map>
#include <boost/function.hpp>

/**
 * A Signal acts as callback relay supporting multiple "subscribers".
 * Whenever the signal gets fired the callbacks are invoked in a specific order.
 *
 * Use the connect() method to add a callback to this signal.
 */
class Signal :
	protected std::map<std::size_t, boost::function<void ()> >
{
public:
	typedef boost::function<void ()> FunctionType;
	typedef std::map<std::size_t, FunctionType> BaseType;

private:
	std::size_t _nextHandle;

public:
	Signal() :
		_nextHandle(0)
	{}

	void operator()() const
	{
		for (BaseType::const_iterator i = BaseType::begin(); i != BaseType::end(); )
		{
			(i++)->second();
		}
	}

	/**
	 * Adds a new callback at the end of the signal's subscriber list.
	 */
	std::size_t connect(const FunctionType& type)
	{
		// Prevent duplicates in debug builds
		std::pair<BaseType::iterator, bool> result = BaseType::insert(
			BaseType::value_type(_nextHandle++, type)
		);

		return result.first->first;
	}

	/** 
	 * Removes the given callback from the list. 
	 * It is safe to call this when the signal is currently emitted.
	 */
	void disconnect(std::size_t handle)
	{
		BaseType::iterator found = BaseType::find(handle);
		
		// Force valid calls in debug builds
		assert(found != BaseType::end());

		if (found != BaseType::end())
		{
			BaseType::erase(found);
		}
	}
};

/**
 * A signal whose subscribers accept one argument type.
 */
template<typename FirstArgument>
class Signal1 :
	protected std::map<std::size_t, boost::function<void (FirstArgument)> >
{
public:
	typedef boost::function<void (FirstArgument)> FunctionType;
	typedef std::map<std::size_t, FunctionType> BaseType;

private:
	std::size_t _nextHandle;

public:
	Signal1() :
		_nextHandle(0)
	{}

	void operator()(FirstArgument arg) const
	{
		for (typename BaseType::const_iterator i = BaseType::begin(); i != BaseType::end(); )
		{
			(i++)->second(arg);
		}
	}

	/**
	 * Adds a new callback at the end of the signal's subscriber list.
	 */
	std::size_t connect(const FunctionType& type)
	{
		// Prevent duplicates in debug builds
		std::pair<typename BaseType::iterator, bool> result = BaseType::insert(
			typename BaseType::value_type(_nextHandle++, type)
		);

		return result.first->first;
	}

	/** 
	 * Removes the given callback from the list. 
	 * It is safe to call this when the signal is currently emitted.
	 */
	void disconnect(std::size_t handle)
	{
		typename BaseType::iterator found = BaseType::find(handle);
		
		// Force valid calls in debug builds
		assert(found != BaseType::end());

		if (found != BaseType::end())
		{
			BaseType::erase(found);
		}
	}
};

#endif /* _SIGNAL_H_ */
