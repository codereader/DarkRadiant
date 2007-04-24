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
	// The quit callback handle
	guint _quitHandle;

protected:
	// The variable that should hold the main widget 
	GtkWidget* _widget;
	
public:
	/** greebo: The constructor sets up the GTK quit callback
	 * 			that should be called right before the main
	 * 			loop exits to invalidate the widget pointer. 
	 */
	PropertyEditor();

	/** greebo: Default destructor, checks for a valid widget
	 * 			and destroys it if not yet done.
	 */
	virtual ~PropertyEditor();

	/**
	 * Return the GtkWidget which is packed into the EntityInspector to display
	 * the PropertyEditor.
	 */
	virtual GtkWidget* getWidget();
    
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
  
	/** greebo: This function gets called right before the main loop
	 * 			quits and gives this class a chance to invalidate their
	 * 			widget pointers so that they don't get destroyed twice.  			
	 */
	static gboolean onGTKQuit(gpointer data);  
};

}

#endif /*PROPERTYEDITOR_H_*/
