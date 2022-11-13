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
 * Represents the key selection in the entity inspector.
 * This can be the direct key like "model" or it can be a proxy setter,
 * in the form of "set <key> on <attachment>" which are propagating
 * key values to attached entities at spawn time.
 */
class ITargetKey
{
public:
    using Ptr = std::shared_ptr<ITargetKey>;

    virtual ~ITargetKey() {}

    // Returns the full string representation of this key, e.g. "classname" or "set _color on flame"
    virtual std::string getFullKey() const = 0;

    // The key this is affecting, either on this entity or an attachment
    // ("set X on Y" style keys will return key part X).
    virtual const std::string& getAffectedKey() const = 0;

    // Modify the key this setter is affecting.
    // Passing an empty key will throw an std::invalid_argument exception.
    virtual void setAffectedKey(const std::string&) = 0;

    // Returns true whether this is a "set X on Y" style key, or false if this a regular key
    virtual bool isTargetingAttachment() const = 0;

    // In case this a "set x on y" style key, this method returns the name of the attachment y
    virtual const std::string& getAttachmentName() const = 0;

    // Set the name of the attachment to modify. Assigning a non-empty name will
    // make this key a "set x on y" setter, assigning an name will
    // convert this to a regular entity key.
    virtual void setAttachmentName(const std::string& name) = 0;

    // Returns a new copy of this instance
    virtual Ptr clone() const = 0;
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
     */
    using CreationFunc = std::function<Ptr(wxWindow*, IEntitySelection&, const ITargetKey::Ptr& key)>;

    virtual ~IPropertyEditor() {}

	/**
	 * greebo: Retrieve the widget for packing this into a parent container.
	 */
	virtual wxPanel* getWidget() = 0;

	/**
	 * Instructs the editor to update its widgets from the edited entity's key values.
	 */
	virtual void updateFromEntities() = 0;

    /**
     * A signal that is emitted when a key value has been applied to one or more selected entities.
     * (This is used as a feedback channel for the EntityInspector UI to get notified when
     *  a value has been set, such that the text entry boxes can follow along.)
     */
    virtual sigc::signal<void(const std::string&, const std::string&)>& signal_keyValueApplied() = 0;
};

class IEntityInspectorModule :
	public RegisterableModule
{
public:
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
};

} // namespace ui

constexpr const char* const MODULE_ENTITYINSPECTOR("EntityInspector");

inline ui::IEntityInspectorModule& GlobalEntityInspector()
{
	static module::InstanceReference<ui::IEntityInspectorModule> _reference(MODULE_ENTITYINSPECTOR);
	return _reference;
}
