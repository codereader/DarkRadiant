#pragma once

#include "imodule.h"

class Entity;
class wxPanel;
class wxWindow;
class IEntitySelection; // defined in ientity.h

namespace ui
{

/**
 * greebo: Some entity property editors may implement this interface,
 * to allow modules to call their dialog windows directly.
 */
class IPropertyEditorDialog
{
public:
    using Ptr = std::shared_ptr<IPropertyEditorDialog>;
    using CreationFunc = std::function<Ptr()>;

    virtual ~IPropertyEditorDialog() {}

	/**
	 * greebo: Run the dialog, returning the new value for the named property.
	 */
	virtual std::string runDialog(Entity* entity, const std::string& key) = 0;
};

/**
 * Abstract base for a PropertyEditor which provides
 * a user interface for editing spawnargs (entity keyvalues).
 */
class IPropertyEditor
{
public:
    using Ptr = std::shared_ptr<IPropertyEditor>;

    /**
     * Construction function object. This callable returns the new PropertyEditor instance.
     *
     * @param parent
     * The parent window, needed by the code to pack the widgets of this editor.
     *
     * @param entities
     * The Entity Set to interact with.
     *
     * @param key
     * The key name which this PropertyEditor is displaying.
     *
     * @param options
     * PropertyEditor-specific options string, from the .game file.
     */
    using CreationFunc = std::function<Ptr(wxWindow*, IEntitySelection&, 
        const std::string& key, const std::string& options)>;

    virtual ~IPropertyEditor() {}

	/**
	 * greebo: Retrieve the widget for packing this into a parent container.
	 */
	virtual wxPanel* getWidget() = 0;

	/**
	 * Instructs the editor to update its widgets from the edited entity's key values.
	 */
	virtual void updateFromEntities() = 0;
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
	virtual void registerPropertyEditor(const std::string& key, const IPropertyEditor::CreationFunc& create) = 0;

    /**
     * Removes the property editor for the given key.
     */
	virtual void unregisterPropertyEditor(const std::string& key) = 0;

    // Dialog Handling

    virtual void registerPropertyEditorDialog(const std::string& key, const IPropertyEditorDialog::CreationFunc& create) = 0;
    virtual IPropertyEditorDialog::Ptr createDialog(const std::string& key) = 0;
    virtual void unregisterPropertyEditorDialog(const std::string& key) = 0;

	// Lets the EntityInspector restore its settings from the Registry
	virtual void restoreSettings() = 0;
};

} // namespace ui

constexpr const char* const MODULE_ENTITYINSPECTOR("EntityInspector");

inline ui::IEntityInspector& GlobalEntityInspector()
{
	static module::InstanceReference<ui::IEntityInspector> _reference(MODULE_ENTITYINSPECTOR);
	return _reference;
}
