#ifndef _IENTITY_INSPECTOR_H_
#define _IENTITY_INSPECTOR_H_

#include "imodule.h"
#include "gtkutil/ifc/Widget.h"

// Forward decl.
typedef struct _GtkWidget GtkWidget;

class Entity;

namespace ui
{

class IPropertyEditor;
typedef boost::shared_ptr<IPropertyEditor> IPropertyEditorPtr;

/**
 * Abstract base for a PropertyEditor which provides
 * a user interface for editing spawnargs (entity keyvalues).
 */
class IPropertyEditor :
	public gtkutil::Widget
{
public:
    virtual ~IPropertyEditor() {}
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
	public RegisterableModule,
	public gtkutil::Widget
{
public:
	/**
	 * Registers the given property editor and associates it with the given entity key.
	 * (The string key is interpreted as regular expression.)
	 */
	virtual void registerPropertyEditor(const std::string& key, const IPropertyEditorPtr& editor) = 0;

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
