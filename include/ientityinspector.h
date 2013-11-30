#ifndef _IENTITY_INSPECTOR_H_
#define _IENTITY_INSPECTOR_H_

#include "imodule.h"

namespace Gtk
{
	class Widget;
}

class Entity;

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
typedef boost::shared_ptr<IPropertyEditorDialog> IPropertyEditorDialogPtr;

class IPropertyEditor;
typedef boost::shared_ptr<IPropertyEditor> IPropertyEditorPtr;

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
	virtual Gtk::Widget& getWidget() = 0;

	/**
	 * Clone method for virtual construction. This method must create a new
	 * PropertyEditor of the same type as the derive class which is implementing
	 * the method.
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
	virtual IPropertyEditorPtr createNew(Entity* entity,
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
	virtual Gtk::Widget& getWidget() = 0;

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
		*boost::static_pointer_cast<ui::IEntityInspector>(
			module::GlobalModuleRegistry().getModule(MODULE_ENTITYINSPECTOR)
		)
	);
	return _inspector;
}

#endif /* _IENTITY_INSPECTOR_H_ */
