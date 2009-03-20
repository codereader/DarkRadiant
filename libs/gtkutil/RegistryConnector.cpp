#include "RegistryConnector.h"

#include "iregistry.h"
#include <iostream>
#include "gtk/gtktogglebutton.h"
#include "gtk/gtkcombobox.h"
#include "gtk/gtkentry.h"
#include "gtk/gtkspinbutton.h"
#include "gtk/gtkrange.h"
#include "gtk/gtkscale.h"
#include "gtk/gtkradiobutton.h"

namespace gtkutil {

void RegistryConnector::addObject(const std::string& key,
                                  StringSerialisablePtr obj)
{
   assert(_objects.find(key) == _objects.end());
   _objects.insert(ObjectMap::value_type(key, obj));

   // Import initial value from registry
   importRegistryValue(obj, key);
}

void RegistryConnector::importRegistryValue(StringSerialisablePtr obj,
                                            const std::string& key)
{
   obj->importFromString(GlobalRegistry().get(key));
}

void RegistryConnector::importValues() 
{
   for (ObjectMap::iterator i = _objects.begin();
        i != _objects.end();
        ++i)
   {
      importRegistryValue(i->second, i->first);
   }
}

void RegistryConnector::exportValues() {

   for (ObjectMap::const_iterator i = _objects.begin();
        i != _objects.end();
        ++i)
   {
      GlobalRegistry().set(i->first, i->second->exportToString());
   }
}

} // namespace gtkutil
