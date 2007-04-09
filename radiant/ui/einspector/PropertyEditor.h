#ifndef PROPERTYEDITOR_H_
#define PROPERTYEDITOR_H_

#include <gtk/gtkwidget.h>
#include <string>
#include <boost/shared_ptr.hpp>

/* FORWARD DECLS */
class Entity;
namespace ui { class PropertyEditor; }

namespace ui
{

/** 
 * PropertyEditor shared pointer type.
 */
typedef boost::shared_ptr<PropertyEditor> PropertyEditorPtr;

/**
 * Abstract interface class for PropertyEditor widgets.
 */
class PropertyEditor
{
public:

	/**
	 * Return the GtkWidget which is packed into the EntityInspector to display
	 * the PropertyEditor.
	 */
	virtual GtkWidget* getWidget() = 0;
    
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
    virtual PropertyEditorPtr createNew(Entity* entity, 
										const std::string& key,
										const std::string& options) = 0;
    
};

}

#endif /*PROPERTYEDITOR_H_*/
