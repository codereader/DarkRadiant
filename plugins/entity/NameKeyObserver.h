#ifndef _NAME_KEY_OBSERVER_H_
#define _NAME_KEY_OBSERVER_H_

#include "ientity.h"

class INamespace;

namespace entity {

/**
 * greebo: A NameKeyObserver is attached to all EntityKeyValues
 * which qualify as name in the context of the current game.
 *
 * In Doom3, this is the "name" key only. As soon as such a key
 * changes, this class notifies the namespace about that event.
*/
class NameKeyObserver :
	public KeyObserver
{
private:
	EntityKeyValue& _keyValue;

	// The old value, needs to be remembered to notify the namespace
	std::string _oldValue;

	// The namespace we're supposed to notify
	INamespace* _namespace;

public:
	NameKeyObserver(EntityKeyValue& keyValue, INamespace* ns);

	~NameKeyObserver();

	// This gets called when the observed KeyValue changes.
	void onKeyValueChanged(const std::string& newValue);
};
typedef boost::shared_ptr<NameKeyObserver> NameKeyObserverPtr;

} // namespace entity

#endif /* _NAME_KEY_OBSERVER_H_ */
