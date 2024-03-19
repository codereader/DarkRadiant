#pragma once

#include "ieclass.h"
#include "scene/Entity.h"
#include "scene/Entity.h"

namespace entity
{

class NameKey :
	public KeyObserver
{
	// The reference to the spawnarg structure
	Entity& _entity;

	// Cached "name" keyvalue
	std::string _name;

    sigc::signal<void> _sigNameChanged;

public:
    NameKey(Entity& entity) :
		_entity(entity)
	{}

	const std::string& getName() const
	{
		if (_name.empty())
        {
			return _entity.getEntityClass()->getDeclName();
		}
		return _name;
	}

	void onKeyValueChanged(const std::string& value)
	{
		_name = value;

        _sigNameChanged.emit();
	}

    sigc::signal<void>& signal_nameChanged()
    {
        return _sigNameChanged;
    }
};

} // namespace
