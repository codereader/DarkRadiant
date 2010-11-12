#ifndef REGISTRYCONNECTOR_H_
#define REGISTRYCONNECTOR_H_

#include <map>
#include <string>

#include "iregistry.h"
#include <StringSerialisable.h>

namespace gtkutil
{

/**
 * \brief
 * Association of StringSerialisable objects with registry keys.
 *
 * This class maintains a set of serialisable objects, each one associated with
 * a registry key which its value should be written to and read from when the
 * appropriate method is called.
 */
class RegistryConnector
{
   // Association of registry keys with StringSerialisable objects
   typedef std::map<std::string, StringSerialisablePtr> ObjectMap;
   ObjectMap _objects;

public:

   /**
    * \brief
    * Insert a serialisable object to be associated with the given registry
    * key.
    *
    * \param key
    * The registry key to associate the given object with. It is an error to
    * attempt to associate more than one object with the given registry key.
    *
    * \param obj
    * StringSerialisablePtr to a serialisable object whose value will be read
    * and written to the key.
    */
   void addObject(const std::string& key, const StringSerialisablePtr& obj);

	// Loads all the values from the registry into the connected GtkObjects
	void importValues();

	// Reads the values from the GtkObjects and writes them into the XMLRegistry
	void exportValues();

private:

   // Load a value from the registry into the StringSerialisable object
   void importRegistryValue(const StringSerialisablePtr& obj,
                            const std::string& key);

}; // class RegistryConnector

} // namespace gtkutil

#endif /* REGISTRYCONNECTOR_H_ */
