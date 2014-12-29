#ifndef _KEY_OBSERVER_DELEGATE_H_
#define _KEY_OBSERVER_DELEGATE_H_

#include "ientity.h"
#include <functional>

namespace entity
{

// The function type used by KeyObserverDelegate
typedef std::function<void(const std::string&)> KeyObserverFunc;

/**
 * greebo: A KeyObserver wrapping around a function object.
 * The function is called as soon as the onKeyValueChanged
 * event is fired.
 */
class KeyObserverDelegate :
	public KeyObserver
{
private:
	// The callback which is invoked on key value change
	KeyObserverFunc _callback;
public:
	KeyObserverDelegate() :
		_callback(std::bind(&KeyObserverDelegate::emptyCallback, this, std::placeholders::_1))
	{}

	KeyObserverDelegate(const KeyObserverFunc& callback) :
		_callback(callback)
	{}

	void setCallback(const KeyObserverFunc& callback)
	{
		_callback = callback;
	}

	void onKeyValueChanged(const std::string& newValue)
	{
		_callback(newValue);
	}

private:
	void emptyCallback(const std::string& newValue)
	{}
};

} // namespace entity

#endif /* _KEY_OBSERVER_DELEGATE_H_ */
