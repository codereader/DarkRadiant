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

    // Blank ctor for map registration
    PropertyEditor();

	// Constructor
	PropertyEditor(Entity* ent, const std::string& key, const std::string& type);

	// Virtual destructor
	virtual ~PropertyEditor();

    // Retrieve the GtkWidget for this PropertyEditor for inclusion into the
    // EntityInspector dialog.
	GtkWidget* getWidget() {
        return _widget;   
    }
    
    // Create a new PropertyEditor of the same type as the derived class (for
    // virtual construction).
    virtual PropertyEditor* createNew(Entity*, const std::string&) = 0;
    
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

private: // methods

    // Static callbacks for the Apply and Reset buttons. These will eventually 
    // invoke the corresponding virtual functions on the derived class.
    static void callbackApply(GtkWidget*, PropertyEditor*);
    static void callbackReset(GtkWidget*, PropertyEditor*);
    
    // Static callback for the Key Active checkbox, which enables or disables
    // the central edit pane.
    static void callbackActiveToggled(GtkWidget*, PropertyEditor*);


private: // fields

    // Main widget
    GtkWidget* _widget;

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
	
	// The checkbox controlling whether the key should be set on the Entity
	// or not
	GtkWidget* _activeCheckbox;

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
