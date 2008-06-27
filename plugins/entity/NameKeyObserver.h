#ifndef _NAME_KEY_OBSERVER_H_
#define _NAME_KEY_OBSERVER_H_

#include <string>
#include <boost/shared_ptr.hpp>
#include "generic/callback.h"

class INamespace;
class EntityKeyValue;

namespace entity {

/**
 * greebo: A NameKeyObserver is attached to all EntityKeyValues
 * which qualify as name in the context of the current game.
 *
 * In Doom3, this is the "name" key only. As soon as such a key
 * changes, this class notifies the namespace about that event.
 */
class NameKeyObserver
{
	EntityKeyValue& _keyValue;

	// The old value, needs to be remembered to notify the namespace
	std::string _oldValue;

	// The namespace we're supposed to notify
	INamespace* _namespace;

public:
	NameKeyObserver(EntityKeyValue& keyValue, INamespace* ns);

	~NameKeyObserver();

	// A callback compatible with the KeyObserver declaration in ientity.h
	// This gets called when the observed KeyValue changes.
	void onNameChange(const std::string& newValue);

	// Define the callback, which is compatible to the KeyObserver definition
	typedef MemberCaller1<NameKeyObserver, 
		const std::string&, &NameKeyObserver::onNameChange> NameChangeCallback;
};
typedef boost::shared_ptr<NameKeyObserver> NameKeyObserverPtr;

} // namespace entity

#endif /* _NAME_KEY_OBSERVER_H_ */
