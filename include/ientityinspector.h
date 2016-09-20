#pragma once

#include "imodule.h"

class Entity;
class wxPanel;
class wxWindow;

namespace ui
{

/**
 * greebo: Some entity property editors may implement this interface,
 * to allow modules to call their dialog windows directly.
 */
class IPropertyEditorDialog
{
public:
    virtual ~IPropertyEditorDialog() {}

	/**
	 * greebo: Run the dialog, returning the new value for the named property.
	 */
	virtual std::string runDialog(Entity* entity, const std::string& key) = 0;
};
typedef std::shared_ptr<IPropertyEditorDialog> IPropertyEditorDialogPtr;

class IPropertyEditor;
typedef std::shared_ptr<IPropertyEditor> IPropertyEditorPtr;

/**
 * Abstract base for a PropertyEditor which provides
 * a user interface for editing spawnargs (entity keyvalues).
 */
class IPropertyEditor
{
public:
    virtual ~IPropertyEditor() {}

	/**
	 * greebo: Retrieve the widget for packing this into a parent container.
	 */
	virtual wxPanel* getWidget() = 0;

	/**
	 * Instructs the editor to update its widgets from the edited entity's key values.
	 */
	virtual void updateFromEntity() = 0;

	/**
	 * Clone method for virtual construction. This method must create a new
	 * PropertyEditor of the same type as the derive class which is implementing
	 * the method.
	 *
	 * @param parent
	 * The parent window, needed by the code to pack the widgets of this editor.
	 *
	 * @param entity
	 * The Entity to edit.
	 *
	 * @param key
	 * The key name which this PropertyEditor is displaying.
	 *
	 * @param options
	 * PropertyEditor-specific options string, from the .game file.
	 */
	virtual IPropertyEditorPtr createNew(wxWindow* parent,
										Entity* entity,
										const std::string& key,
										const std::string& options) = 0;

};

class IEntityInspector :
	public RegisterableModule
{
public:
	/**
	 * greebo: Retrieve the widget for packing this into a parent container.
	 */
	virtual wxPanel* getWidget() = 0;

	/**
	 * Registers the given property editor and associates it with the given entity key.
	 * (The string key is interpreted as regular expression.)
	 */
	virtual void registerPropertyEditor(const std::string& key, const IPropertyEditorPtr& editor) = 0;

	/**
	 * Looks up a property editor for the given key.
	 */
	virtual IPropertyEditorPtr getRegisteredPropertyEditor(const std::string& key) = 0;

	/**
	 * Removes the property editor for the given key.
	 */
	virtual void unregisterPropertyEditor(const std::string& key) = 0;

	// Lets the EntityInspector restore its settings from the Registry
	virtual void restoreSettings() = 0;
};

} // namespace ui

const std::string MODULE_ENTITYINSPECTOR("EntityInspector");

inline ui::IEntityInspector& GlobalEntityInspector()
{
	// Cache the reference locally
	static ui::IEntityInspector& _inspector(
		*std::static_pointer_cast<ui::IEntityInspector>(
			module::GlobalModuleRegistry().getModule(MODULE_ENTITYINSPECTOR)
		)
	);
	return _inspector;
}
