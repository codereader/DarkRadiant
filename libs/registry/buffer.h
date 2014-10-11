#pragma once

#include <map>
#include <algorithm>
#include "iregistry.h"
#include "string/convert.h"

/// Convenience methods and types for interacting with the XML registry
namespace registry
{

/**
 * A proxy class that is buffering all write operations.
 * Nothing is written to the back-end XMLRegistry until
 * a call to commitChanges() is performed.
 * Only the most common operations are supported by this class,
 * to get and set certain registry key values.
 */
class Buffer
{
private:
	Registry& _backend;

	// A key => value map to store buffered values
	typedef std::map<std::string, std::string> KeyValueBuffer;
	KeyValueBuffer _buffer;

public:
	// Pass the Registry instance to wrap around, defaults to GlobalRegistry()
	Buffer(Registry& backend = GlobalRegistry()) :
		_backend(backend)
	{}

	// Get the value of the given registry key - returns cached values if present
	std::string get(const std::string& key)
	{
		KeyValueBuffer::const_iterator found = _buffer.find(key);

		// Check if the key value is cached
		if (found != _buffer.end())
		{
			return found->second; // return the cached value
		}

		// Not cached, pass the call to the backend
		return _backend.get(key);
	}

	// Set the value of the given registry key
	void set(const std::string& key, const std::string& value)
	{
		_buffer[key] = value;
	}

	// Discard all pending write operations
	void clear()
	{
		_buffer.clear();
	}

	bool keyExists(const std::string& key)
	{
		KeyValueBuffer::const_iterator found = _buffer.find(key);

		// Check if the key value is cached
		if (found != _buffer.end())
		{
			return true;
		}

		return _backend.keyExists(key);
	}

	// Performs any pending write operations
	void commitChanges()
	{
		std::for_each(_buffer.begin(), _buffer.end(), [&] (KeyValueBuffer::value_type& kv)
		{
			_backend.set(kv.first, kv.second);
		});

		_buffer.clear();
	}
};

} // namespace
