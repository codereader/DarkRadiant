#ifndef PROPERTYEDITOR_H_
#define PROPERTYEDITOR_H_

#include "ientity.h"

#include <map>
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
	PropertyEditor(Entity* ent, const std::string& key);

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

private:

	// The Entity to edit
	Entity* _entity;
	
	// The key we are editing on this Entity
	const std::string& _key;

	// The Gtk box containing the apply/reset buttons which are common for all
	// subclasses.
	GtkWidget* _applyButtonHbox;

	// The Gtk box containing the PropertyEditor title text and icon
	GtkWidget* _titleBox;

protected:

	// Return the apply/reset button box. The subclass can choose whether or 
	// not to display this, but it should not modify it.
	GtkWidget* getApplyButtonHbox();
	
	// Return the title bar box
	GtkWidget* getTitleBox();
    
};

}

#endif /*PROPERTYEDITOR_H_*/
