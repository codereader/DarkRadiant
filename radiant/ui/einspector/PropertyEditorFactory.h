#pragma once

#include "PropertyEditor.h"

#include <map>
#include <string>
#include <wx/bitmap.h>

/* FORWARD DECLS */
class IEntitySelection;

namespace ui
{

/**
 * PropertyEditorFactory
 * Factory class to create PropertyEditor instances for given entity keys.
 */
class PropertyEditorFactory final
{
   // Mapping from classnames to PropertyEditor child instances. The type's 
   // CreationFunc object will be used to create a new object
   std::map<std::string, IPropertyEditor::CreationFunc> _peMap;

   // A mapping between (regex) keys and custom editors (registered by plug-ins, etc.)
   std::map<std::string, IPropertyEditor::CreationFunc> _customEditors;

   std::map<std::string, IPropertyEditorDialog::CreationFunc> _dialogs;

public:
    PropertyEditorFactory();
    ~PropertyEditorFactory();

    // Create a new PropertyEditor with the provided classname to manage the
    // given Entity object and key name.
    IPropertyEditor::Ptr create(wxWindow* parent, const std::string& className,
        IEntitySelection& entities, const ITargetKey::Ptr& key);

    // Associate a specific property editor for the given key (regex)
	void registerPropertyEditor(const std::string& key, const IPropertyEditor::CreationFunc& creator);
	void unregisterPropertyEditor(const std::string& key);

    void registerPropertyEditorDialog(const std::string& key, const IPropertyEditorDialog::CreationFunc& create);
    IPropertyEditorDialog::Ptr createDialog(const std::string& key);
    void unregisterPropertyEditorDialog(const std::string& key);

    // Return the bitmap that corresponds to the provided PropertyEditor type.
	static wxBitmap getBitmapFor(const std::string& type);

private:
    // Register the default classes
    void registerBuiltinTypes();
};

}
