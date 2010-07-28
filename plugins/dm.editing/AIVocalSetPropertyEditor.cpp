#include "AIVocalSetPropertyEditor.h"

#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkbutton.h>

#include "i18n.h"
#include "ieclass.h"
#include "iuimanager.h"
#include "ientity.h"

#include "AIVocalSetChooserDialog.h"

namespace ui
{

AIVocalSetPropertyEditor::AIVocalSetPropertyEditor() :
	_widget(NULL),
	_entity(NULL)
{}

AIVocalSetPropertyEditor::AIVocalSetPropertyEditor(Entity* entity, const std::string& key, const std::string& options) :
	_entity(entity)
{
	_widget = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(_widget), 6);

	// Horizontal box contains the browse button
	GtkWidget* hbx = gtk_hbox_new(FALSE, 3);
	gtk_container_set_border_width(GTK_CONTAINER(hbx), 3);

	// Browse button for models
	GtkWidget* browseButton = gtk_button_new_with_label(_("Select Vocal Set..."));
	gtk_button_set_image(
		GTK_BUTTON(browseButton),
		gtk_image_new_from_pixbuf(
			GlobalUIManager().getLocalPixbuf("icon_sound.png")->gobj()
		)
	);
	g_signal_connect(G_OBJECT(browseButton), "clicked", G_CALLBACK(onChooseButton), this);

	gtk_box_pack_start(GTK_BOX(hbx), browseButton, TRUE, FALSE, 0);

	// Pack hbox into vbox (to limit vertical size), then edit frame
	GtkWidget* vbx = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), hbx, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(_widget), vbx, TRUE, TRUE, 0);
}

IPropertyEditorPtr AIVocalSetPropertyEditor::createNew(Entity* entity, 
	const std::string& key, const std::string& options)
{
	return IPropertyEditorPtr(new AIVocalSetPropertyEditor(entity, key, options));
}

void AIVocalSetPropertyEditor::onChooseButton(GtkWidget* button, AIVocalSetPropertyEditor* self)
{
	// Construct a new vocal set chooser dialog
	AIVocalSetChooserDialog dialog;

	dialog.setSelectedVocalSet(self->_entity->getKeyValue(DEF_VOCAL_SET_KEY));

	// Show and block
	dialog.show();

	if (dialog.getResult() == AIVocalSetChooserDialog::RESULT_OK)
	{
		self->_entity->setKeyValue(DEF_VOCAL_SET_KEY, dialog.getSelectedVocalSet());
	}
}

} // namespace ui
