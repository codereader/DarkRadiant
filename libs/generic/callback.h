#ifndef _CALLBACK_H_
#define _CALLBACK_H_

#include <boost/function.hpp>

/**
 * Safe callback based on boost::function, same behaviour as the old Callback template:
 * doesn't crash or throw when the function pointer is empty. 
 * Can be replaced with boost::function<void()> as soon as all client code has been 
 * adjusted to handle empty callback objects on their own.
 */
class Callback
{
private:
	typedef boost::function<void()> FunctionType;
	FunctionType _func;

public:
	Callback()
	{}

	Callback(const FunctionType& func) :
		_func(func)
	{}

	Callback& operator=(const FunctionType& rhs)
	{
		_func = rhs;
		return *this;
	}

	bool empty() const
	{
		return _func.empty();
	}

// Safe-bool idiom, copied from boost::function template
private:
    struct dummy {
		void nonnull() {};
    };

    typedef void (dummy::*safe_bool)();

public:
    operator safe_bool () const
	{ 
		return (_func.empty()) ? NULL : &dummy::nonnull;
	}

	bool operator!() const
	{
		return _func.empty();
	}

	// safe invocation, doesn't call function if empty
	void operator()()
	{
		if (_func)
		{
			_func();
		}
	}
};

#endif /* _CALLBACK_H_ */
