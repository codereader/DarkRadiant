#pragma once

#include <map>
#include "iregistry.h"
#include "string/convert.h"
#include <glibmm/propertyproxy.h>

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

// As in bind.h it's possible to bind certain Glib properties to registry keys
// while using the registry::Buffer as target.
namespace detail
{

template<typename T>
void savePropertyToBuffer(Glib::PropertyProxy<T>& prop,
						  Buffer& buffer, 
						  const std::string& rkey)
{
    buffer.set(rkey, string::to_string(prop.get_value()));
}

template<typename T>
void resetPropertyToRegistryKey(Glib::PropertyProxy<T> prop, const std::string& key, Buffer& buffer)
{
    // Set initial value then connect to changed signal
    if (buffer.keyExists(key))
    {
        prop.set_value(string::convert<T>(buffer.get(key)));
    }
}

} // namespace

template<typename T>
void bindPropertyToBufferedKey(Glib::PropertyProxy<T> prop, const std::string& key, 
							   Buffer& buffer, sigc::signal<void>& resetSignal)
{
	// Set initial value then connect to changed signal
    detail::resetPropertyToRegistryKey(prop, key, buffer);

    prop.signal_changed().connect(
        sigc::bind(sigc::ptr_fun(detail::savePropertyToBuffer<T>), prop, sigc::ref(buffer), key)
    );

	resetSignal.connect(
		sigc::bind(sigc::ptr_fun(detail::resetPropertyToRegistryKey<T>), prop, key, sigc::ref(buffer))
	);
}

} // namespace
