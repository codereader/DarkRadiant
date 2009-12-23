#include "EntityPropertyEditor.h"

#include "iscenegraph.h"
#include "ientity.h"
#include "scenelib.h"

#include "PropertyEditorFactory.h"
#include <gtk/gtk.h>

#include "ui/common/EntityChooser.h"

namespace ui {

// Blank ctor
EntityPropertyEditor::EntityPropertyEditor() :
	PropertyEditor()
{}

// Constructor. Create the GTK widgets here

EntityPropertyEditor::EntityPropertyEditor(Entity* entity, const std::string& name) :
	PropertyEditor(entity),
	_key(name)
{
	_widget = gtk_vbox_new(FALSE, 0);

	// Horizontal box contains the browse button
	GtkWidget* hbx = gtk_hbox_new(FALSE, 3);
	gtk_container_set_border_width(GTK_CONTAINER(hbx), 3);
	
	// Browse button
	GtkWidget* browseButton = gtk_button_new_with_label(
		"Choose target entity..."
	);
	gtk_button_set_image(
		GTK_BUTTON(browseButton),
		gtk_image_new_from_pixbuf(
			PropertyEditorFactory::getPixbufFor("entity")
		)
	);
			
	g_signal_connect(G_OBJECT(browseButton), 
					 "clicked", 
					 G_CALLBACK(_onBrowseButton), 
					 this);
	gtk_box_pack_start(GTK_BOX(hbx), browseButton, TRUE, FALSE, 0);
	
	// Pack hbox into vbox (to limit vertical size), then edit frame
	GtkWidget* vbx = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), hbx, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(_widget), vbx, TRUE, TRUE, 0);
}

/* GTK CALLBACKS */

void EntityPropertyEditor::_onBrowseButton(GtkWidget* w, EntityPropertyEditor* self)
{
	// Use a new dialog window to get a selection from the user
	std::string selection = EntityChooser::ChooseEntity(self->_entity->getKeyValue(self->_key));

	// Only apply non-empty selections if the classname has actually changed
	if (!selection.empty() && selection != self->_entity->getKeyValue(self->_key))
	{
		UndoableCommand cmd("changeKeyValue");

		// Apply the change
		self->_entity->setKeyValue(self->_key, selection);
	}
}

} // namespace ui
