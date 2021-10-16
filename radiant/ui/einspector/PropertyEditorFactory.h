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
   typedef std::map<std::string, IPropertyEditor::CreationFunc> PropertyEditorMap;
   static PropertyEditorMap _peMap;

   // A mapping between (regex) keys and custom editors (registered by plug-ins, etc.)
   static PropertyEditorMap _customEditors;

   static std::map<std::string, IPropertyEditorDialog::CreationFunc> _dialogs;

public:

    // Create a new PropertyEditor with the provided classname to manage the
    // given Entity object and key name.
    static IPropertyEditor::Ptr create(wxWindow* parent, const std::string& className,
        IEntitySelection& entities, const std::string& key, const std::string& options);

    // Register the classes
    static void registerClasses();

	// Associate a specific property editor for the given key (regex)
	static void registerPropertyEditor(const std::string& key, const IPropertyEditor::CreationFunc& creator);

	static void unregisterPropertyEditor(const std::string& key);

    static void registerPropertyEditorDialog(const std::string& key, const IPropertyEditorDialog::CreationFunc& create);
    static IPropertyEditorDialog::Ptr createDialog(const std::string& key);
    static void unregisterPropertyEditorDialog(const std::string& key);

    // Return the bitmap that corresponds to the provided PropertyEditor type.
	static wxBitmap getBitmapFor(const std::string& type);
};

}
