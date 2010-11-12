#include "NameKeyObserver.h"

#include "inamespace.h"
#include "ientity.h"

namespace entity {

NameKeyObserver::NameKeyObserver(EntityKeyValue& keyValue, INamespace* ns) :
	_keyValue(keyValue),
	_namespace(ns)
{
	assert(_namespace != NULL);

	_oldValue = keyValue.get();
	_keyValue.attach(*this);
}

NameKeyObserver::~NameKeyObserver()
{
	_keyValue.detach(*this);
}

void NameKeyObserver::onKeyValueChanged(const std::string& newValue)
{
	assert(_namespace != NULL);

	if (!newValue.empty()) {
		// Notify the namespace, so that it can broadcast the event
		// only do this for non-empty names (empty names mean key deletion, which is handled elsewhere)
		_namespace->nameChanged(_oldValue, newValue);
	}

	// Remember the new value
	_oldValue = newValue;
}

} // namespace entity
