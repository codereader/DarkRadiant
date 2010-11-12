#ifndef _KEY_VALUE_OBSERVER_H_
#define _KEY_VALUE_OBSERVER_H_

#include "ientity.h"
#include <boost/shared_ptr.hpp>

class INamespace;

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
class KeyValueObserver :
	public KeyObserver
{
private:
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

	// This gets called when the observed KeyValue changes.
	void onKeyValueChanged(const std::string& newValue);
};
typedef boost::shared_ptr<KeyValueObserver> KeyValueObserverPtr;

} // namespace entity

#endif /* _KEY_VALUE_OBSERVER_H_ */
