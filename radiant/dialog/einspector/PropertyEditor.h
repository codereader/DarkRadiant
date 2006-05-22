#ifndef PROPERTYEDITOR_H_
#define PROPERTYEDITOR_H_

#include "ientity.h"

#include <map>
#include <string>
#include "gtk/gtk.h"

namespace ui
{

/* PropertyEditor base class.
 * 
 * This class defines the interface for PropertyEditor subclasses which are 
 * responsible for presenting a GTK dialog for editing a particular property
 * on an Entity.
 */

class PropertyEditor
{
public:

	// Constructor
	PropertyEditor(Entity* ent, const std::string& key, const std::string& type);

	// Virtual destructor
	virtual ~PropertyEditor() {}

    // Retrieve the GtkWidget for this PropertyEditor for inclusion into the
    // EntityInspector dialog.
	virtual GtkWidget* getWidget() = 0;
    
    // Create a new PropertyEditor of the same type as the derived class (for
    // virtual construction).
    virtual PropertyEditor* createNew(Entity*, const std::string&) = 0;
    
    // Refresh the PropertyEditor with updated keyvals on the owned Entity.
    virtual void refresh() = 0;
    
    // Apply the PropertyEditor's changes to the owned Entity.
    virtual void commit() = 0;

	// Static callbacks for the Apply and Reset buttons. These will just invoke
	// the corresponding virtual functions on the derived class.
	static void callbackApply(GtkWidget*, gpointer);
	static void callbackReset(GtkWidget*, gpointer);

private:

	// The Entity to edit
	Entity* _entity;
	
	// The key we are editing on this Entity
	const std::string _key;

	// The type of the key we are editing. This is passed up from the derived
	// class constructor based on the type of the derived class.	
	const std::string _type;

	// The Gtk box containing the apply/reset buttons which are common for all
	// subclasses.
	GtkWidget* _applyButtonHbox;

	// The Gtk box containing the PropertyEditor title text and icon
	GtkWidget* _titleBox;
	
	// The central GtkScrolledWindow where the actual editing takes place
	GtkWidget* _editWindow;

protected:

	// Accessor functions for subclasses. 

	// Return the apply/reset button box. 
	GtkWidget* getApplyButtonHbox();
	
	// Return the title bar box
	GtkWidget* getTitleBox();
    
    // Return the central edit window
    GtkWidget* getEditWindow();

	// Get the key and the Entity
	const std::string& getKey();
	Entity* getEntity();
    
};

}

#endif /*PROPERTYEDITOR_H_*/
