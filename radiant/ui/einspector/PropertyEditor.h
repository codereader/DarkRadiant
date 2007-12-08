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
protected:
	
	/**
	 * Return the derived class' main widget to the parent class, so it can
	 * be shown and returned to the EntityInspector. 
	 * 
	 * This separates interface from implementation by allowing child classes to 
	 * return their main widget any way they wish (create-on-demand, private 
	 * member, etc).
	 */
	virtual GtkWidget* _getInternalWidget() = 0;
	
public:
	
	/**
	 * Return the GtkWidget which is packed into the EntityInspector to display
	 * the PropertyEditor.
	 */
	GtkWidget* getWidget();
    
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
