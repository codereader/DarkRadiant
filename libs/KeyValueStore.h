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

	virtual void clearProperties() override
	{
		_keyValues.clear();
	}

	virtual std::string getProperty(const std::string& key, const std::string& value) const override
	{
		auto existing = _keyValues.find(key);

		return existing != _keyValues.end() ? existing->second : std::string();
	}

	virtual void setProperty(const std::string& key, const std::string& value) override
	{
		if (value.empty())
		{
			removeProperty(key);
			return;
		}

		_keyValues[key] = value;
	}

	virtual void removeProperty(const std::string& key) override
	{
		_keyValues.erase(key);
	}

	virtual void foreachProperty(const std::function<void(const std::string&, const std::string&)>& visitor) const override
	{
		for (const auto& pair : _keyValues)
		{
			visitor(pair.first, pair.second);
		}
	}
};
