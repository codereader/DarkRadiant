#ifndef _KEY_VALUE_OBSERVER_H_
#define _KEY_VALUE_OBSERVER_H_

#include <string>
#include <boost/shared_ptr.hpp>
#include "generic/callback.h"

class INamespace;
class EntityKeyValue;

namespace entity {

/**
 * greebo: This class observers a single EntityKeyValue and catches
 * any values that refer to a name in the attached namespace.
 *
 * During an EntityKeyValue's lifetime, the value might be changed
 * to another entity's name. This case needs to be detected and an
 * observer relationship needs to be established between the KeyValue 
 * and the Namespace, which is what this class takes care of.
 */
class KeyValueObserver
{
	EntityKeyValue& _keyValue;

	// The namespace we're supposed to notify
	INamespace* _namespace;

	// Is TRUE while the EntityKeyValue is an active NameObserver
	bool _observing;

	// This is needed to de-register the value on change again
	std::string _observedValue;

public:
	KeyValueObserver(EntityKeyValue& keyValue, INamespace* ns);

	~KeyValueObserver();

	// A callback compatible with the KeyObserver declaration in ientity.h
	// This gets called when the observed KeyValue changes.
	void onValueChange(const std::string& newValue);

	// Define the callback, which is compatible to the KeyObserver definition
	typedef MemberCaller1<KeyValueObserver, 
		const std::string&, &KeyValueObserver::onValueChange> ValueChangeCallback;
};
typedef boost::shared_ptr<KeyValueObserver> KeyValueObserverPtr;

} // namespace entity

#endif /* _KEY_VALUE_OBSERVER_H_ */
