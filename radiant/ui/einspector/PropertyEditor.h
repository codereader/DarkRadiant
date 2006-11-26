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
    // Main widget
    GtkWidget* _widget;

	// The Entity to edit
	Entity* _entity;
	
	// The key we are editing on this Entity
	const std::string _key;

	// The Gtk box containing the apply/reset buttons which are common for all
	// subclasses.
	GtkWidget* _applyButtonHbox;

	// The central GtkScrolledWindow where the actual editing takes place
	GtkWidget* _editWindow;
	
private:

    // Static callback for the Apply button. 
    static void callbackApply(GtkWidget*, PropertyEditor*);

public:

    // Blank ctor for map registration
    PropertyEditor();

	// Constructor
	PropertyEditor(Entity* ent, const std::string& key);

	// Virtual destructor
	virtual ~PropertyEditor();

    // Retrieve the GtkWidget for this PropertyEditor for inclusion into the
    // EntityInspector dialog.
	GtkWidget* getWidget() {
        return _widget;   
    }
    
    // Create a new PropertyEditor of the same type as the derived class (for
    // virtual construction).
    virtual PropertyEditor* createNew(Entity* entity, 
    								   const std::string& key,
    								   const std::string& options) = 0;
    
    // Update the contained widgets with the given key value. This function is
    // always called from the parent PropertyEditor class.
    virtual void setValue(const std::string&) = 0;
    
    // Return the keyvalue as currently specified by the contained Gtk widgets.
    // This function will be called from the parent PropertyEditor class.
    virtual const std::string getValue() = 0;

    // Non-virtual parent class function to obtain the current value of the key
    // from the Entity itself, and invoke the child's setValue() function with
    // the new value. This function exists so that the Gtk widgets can be updated
    // immediately after construction, which would otherwise require a manual
    // call to the callbackReset() GTK callback.
    void refresh();

protected:

	// Accessor functions for subclasses. 

	// Return the apply/reset button box. 
	GtkWidget* getApplyButtonHbox();
	
    // Return the central edit window
    GtkWidget* getEditWindow();

	// Get the key and the Entity
	const std::string& getKey();
	Entity* getEntity();
    
};

}

#endif /*PROPERTYEDITOR_H_*/
