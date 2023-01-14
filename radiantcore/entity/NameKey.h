#pragma once

#include "ieclass.h"
#include "ientity.h"
#include "SpawnArgs.h"

namespace entity 
{

class NameKey :
	public KeyObserver
{
	// The reference to the spawnarg structure
	SpawnArgs& _entity;

	// Cached "name" keyvalue
	std::string _name;

    sigc::signal<void> _sigNameChanged;

public:
    NameKey(SpawnArgs& entity) :
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
