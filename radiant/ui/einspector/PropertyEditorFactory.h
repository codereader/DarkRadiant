#pragma once

#include "PropertyEditor.h"

#include <map>
#include <string>
#include <wx/bitmap.h>

/* FORWARD DECLS */
class Entity;

namespace ui
{

/* PropertyEditorFactory
 *
 * Factory class to create PropertyEditor child classes based on the classnames
 * provided at runtime.
 */

class PropertyEditorFactory
{

   // Mapping from classnames to PropertyEditor child instances. The child
   // instance's createNew() function will be used to create a new object of
   // the correct type.
   typedef std::map<std::string, IPropertyEditorPtr> PropertyEditorMap;
   static PropertyEditorMap _peMap;

   // A mapping between (regex) keys and custom editors (registered by plug-ins, etc.)
   static PropertyEditorMap _customEditors;

public:

    // Create a new PropertyEditor with the provided classname to manage the
    // given Entity object and key name.
    static IPropertyEditorPtr create(wxWindow* parent, const std::string& className,
    								Entity* entity,
    								const std::string& key,
    								const std::string& options);

    // Register the classes
    static void registerClasses();

	// Associate a specific property editor for the given key (regex)
	static void registerPropertyEditor(const std::string& key, const IPropertyEditorPtr& editor);

	static IPropertyEditorPtr getRegisteredPropertyEditor(const std::string& key);

	static void unregisterPropertyEditor(const std::string& key);

    // Return the bitmap that corresponds to the provided PropertyEditor type.
	static wxBitmap getBitmapFor(const std::string& type);
};

}
