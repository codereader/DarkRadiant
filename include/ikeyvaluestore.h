#pragma once

#include <string>
#include <functional>

/**
 * Interface for an object supporting storage of string-valued
 * properties which can be accessed by a key.
 * The key is treated case-sensitively.
 * It's not possible to store empty strings in the store.
*/
class IKeyValueStore
{
public:
	virtual ~IKeyValueStore() {}

	/**
	 * Retrieves a property value from the store. This will return an empty string
	 * if the property doesn't exist.
	 */
	virtual std::string getProperty(const std::string& key) const = 0;

	/**
	 * Store a value in the map root's property store. DarkRadiant will persist
	 * these values in the Info file when saving and restore them on loading.
	 * Other map formats without info file are free to use that information
	 * when exporting map data.
	 * Setting a key to an empty string is equivalent to removing it from the store.
	 */
	virtual void setProperty(const std::string& key, const std::string& value) = 0;

	/**
	 * Removes a property from the value store, if it exists. This is equivalent
	 * to setting a property value to an empty string.
	 */
	virtual void removeProperty(const std::string& key) = 0;

	/**
	 * Iterates over the property store, hitting the visitor with each value pair.
	 */
	virtual void foreachProperty(const std::function<void (const std::string&, const std::string&)>& visitor) const = 0;

	/**
	 * Removes all properties from this store
	 */
	virtual void clearProperties() = 0;
};

