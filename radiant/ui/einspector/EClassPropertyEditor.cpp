#include "EClassPropertyEditor.h"

#include "ieclass.h"
#include "ientity.h"
#include "scenelib.h"

#include <gtk/gtk.h>

namespace ui
{

// Blank ctor
EClassPropertyEditor::EClassPropertyEditor() :
	ComboBoxPropertyEditor()
{ }

// Constructor. Create the GTK widgets here
EClassPropertyEditor::EClassPropertyEditor(Entity* entity, 
										   const std::string& name) 
: ComboBoxPropertyEditor(entity, name)
{
	// Enable the GTK idle callback to populate the list
	enableIdleCallback();
}

namespace {
	
class EClassPopulator :
	public EntityClassVisitor
{
	// List store to add to
	GtkTreeModel* _store;
	
public:
	// Constructor
	EClassPopulator(GtkWidget* box) {
        _store = gtk_combo_box_get_model(GTK_COMBO_BOX(box));
    }
	
	virtual void visit(IEntityClassPtr eclass) {
		// Append the name to the list store
        GtkTreeIter iter;
        gtk_list_store_append(GTK_LIST_STORE(_store), &iter);
        gtk_list_store_set(GTK_LIST_STORE(_store), &iter, 
        				   0, eclass->getName().c_str(), 
        				   -1);
	}
};
}

// Traverse the scenegraph to populate the combo box
void EClassPropertyEditor::onGtkIdle() {
	// Construct an eclass visitor and traverse the eclasses
	EClassPopulator populator(_comboBox);
	GlobalEntityClassManager().forEach(populator);
}

} // namespace ui
