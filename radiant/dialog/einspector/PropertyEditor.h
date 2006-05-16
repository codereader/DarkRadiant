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
    
};

}

#endif /*PROPERTYEDITOR_H_*/
