#include "AIHeadPropertyEditor.h"

#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkbutton.h>

#include "i18n.h"
#include "ieclass.h"
#include "iuimanager.h"
#include "ientity.h"

#include "AIHeadChooserDialog.h"

namespace ui
{

AIHeadPropertyEditor::AIHeadPropertyEditor() :
	_widget(NULL),
	_entity(NULL)
{}

AIHeadPropertyEditor::AIHeadPropertyEditor(Entity* entity, const std::string& key, const std::string& options) :
	_entity(entity)
{
	_widget = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(_widget), 6);

	// Horizontal box contains the browse button
	GtkWidget* hbx = gtk_hbox_new(FALSE, 3);
	gtk_container_set_border_width(GTK_CONTAINER(hbx), 3);

	// Browse button for models
	GtkWidget* browseButton = gtk_button_new_with_label(_("Choose AI head..."));
	gtk_button_set_image(
		GTK_BUTTON(browseButton),
		gtk_image_new_from_pixbuf(
			GlobalUIManager().getLocalPixbuf("icon_model.png")
		)
	);
	g_signal_connect(G_OBJECT(browseButton), "clicked", G_CALLBACK(onChooseButton), this);

	gtk_box_pack_start(GTK_BOX(hbx), browseButton, TRUE, FALSE, 0);

	// Pack hbox into vbox (to limit vertical size), then edit frame
	GtkWidget* vbx = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), hbx, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(_widget), vbx, TRUE, TRUE, 0);
}

IPropertyEditorPtr AIHeadPropertyEditor::createNew(Entity* entity, 
	const std::string& key, const std::string& options)
{
	return IPropertyEditorPtr(new AIHeadPropertyEditor(entity, key, options));
}

void AIHeadPropertyEditor::onChooseButton(GtkWidget* button, AIHeadPropertyEditor* self)
{
	// Construct a new head chooser dialog
	AIHeadChooserDialog dialog;

	dialog.setSelectedHead(self->_entity->getKeyValue(DEF_HEAD_KEY));

	// Show and block
	dialog.show();

	if (dialog.getResult() == AIHeadChooserDialog::RESULT_OK)
	{
		self->_entity->setKeyValue(DEF_HEAD_KEY, dialog.getSelectedHead());
	}
}

} // namespace ui
