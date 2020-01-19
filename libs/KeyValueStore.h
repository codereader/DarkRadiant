#pragma once

#include "ikeyvaluestore.h"

#include <map>
#include <string>

/**
 * Default implementation of a key/value store by use of std::map.
 */
class KeyValueStore :
	public virtual IKeyValueStore
{
private:
	typedef std::map<std::string, std::string> KeyValues;
	KeyValues _keyValues;

public:
	std::size_t getPropertyCount() const
	{
		return _keyValues.size();
	}

	void clearProperties()
	{
		_keyValues.clear();
	}

	virtual std::string getProperty(const std::string& key, const std::string& value) const
	{
		auto existing = _keyValues.find(key);

		return existing != _keyValues.end() ? existing->second : std::string();
	}

	virtual void setProperty(const std::string& key, const std::string& value)
	{
		if (value.empty())
		{
			removeProperty(key);
			return;
		}

		_keyValues[key] = value;
	}

	virtual void removeProperty(const std::string& key)
	{
		_keyValues.erase(key);
	}

	virtual void foreachProperty(const std::function<void(const std::string&, const std::string&)>& visitor) const
	{
		for (const auto& pair : _keyValues)
		{
			visitor(pair.first, pair.second);
		}
	}
};
