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
    typedef std::map<const std::string, const PropertyEditor*> PropertyEditorMap;

    // Mapping of class names to actual PropertyEditor objects (for virtual
    // construction
    static PropertyEditorMap _propertyEditorMap;
    
public:

    // Retrieve the GtkWidget for this PropertyEditor for inclusion into the
    // EntityInspector dialog.
	virtual GtkWidget* getWidget() = 0;
    
    // Create a new PropertyEditor of the same type as the derived class (for
    // virtual construction).
    virtual PropertyEditor* createNew(Entity*, const char*) = 0;
    
    // Refresh the PropertyEditor with updated keyvals on the owned Entity.
    virtual void refresh() = 0;
    
    // Apply the PropertyEditor's changes to the owned Entity.
    virtual void commit() = 0;
    
    // Static function to instantiate a derived PropertyEditor subclass based
    // on the text name supplied at runtime.
    static PropertyEditor* create(Entity* entity, const char* name);
    
    // Static function to add a PropertyEditor subclass to the internal map
    static void registerClass(const char* name, const PropertyEditor* pe);
    
};

}

#endif /*PROPERTYEDITOR_H_*/
