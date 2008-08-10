#include "MissionLogicDialog.h"
#include "ObjectiveEntity.h"

#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/RightAlignment.h"

#include <gtk/gtk.h>

namespace objectives {

namespace {

	const char* const DIALOG_TITLE = "Edit Mission Logic";

	// Widget enum
	enum {
		WIDGET_SUCCESS_LOGIC,
		WIDGET_FAILURE_LOGIC,
	};
}

// Main constructor
MissionLogicDialog::MissionLogicDialog(GtkWindow* parent, ObjectiveEntity& objectiveEnt) :
	gtkutil::BlockingTransientWindow(DIALOG_TITLE, parent),
	_objectiveEnt(objectiveEnt)
{
	// Dialog contains list view, edit panel and buttons
	GtkWidget* vbx = gtk_vbox_new(FALSE, 12);
	
	gtk_box_pack_start(
		GTK_BOX(vbx), gtkutil::LeftAlignedLabel("<b>Components</b>"), FALSE, FALSE, 0
	);

	GtkWidget* compvbox = gtk_vbox_new(FALSE, 6);
	//gtk_box_pack_start(GTK_BOX(compvbox), createListView(), TRUE, TRUE, 0);
	//gtk_box_pack_start(GTK_BOX(compvbox), createEditPanel(), FALSE, FALSE, 0);
	//gtk_box_pack_start(GTK_BOX(compvbox), createComponentEditorPanel(), TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(vbx), gtkutil::LeftAlignment(compvbox, 12, 1.0f) , TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(vbx), gtk_hseparator_new(), FALSE, FALSE, 0);
	//gtk_box_pack_end(GTK_BOX(vbx), createButtons(), FALSE, FALSE, 0);
	
	// Populate the list of components
	//populateObjectiveEditPanel();
	//populateComponents();

	// Add contents to main window
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_container_add(GTK_CONTAINER(getWindow()), vbx);
}

// Create buttons
GtkWidget* MissionLogicDialog::createButtons() {
	// Create a new homogeneous hbox
	GtkWidget* hbx = gtk_hbox_new(TRUE, 6);

	GtkWidget* saveButton = gtk_button_new_from_stock(GTK_STOCK_SAVE);
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	
	g_signal_connect(G_OBJECT(saveButton), "clicked", G_CALLBACK(_onSave), this);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(_onCancel), this);
	
	gtk_box_pack_end(GTK_BOX(hbx), cancelButton, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbx), saveButton, TRUE, TRUE, 0);

	return gtkutil::RightAlignment(hbx);
}

void MissionLogicDialog::save() {
	// TODO
}

// GTK CALLBACKS

// Save button
void MissionLogicDialog::_onSave(GtkWidget* w, MissionLogicDialog* self) {
    self->save();
	self->destroy();
}

// Cancel button
void MissionLogicDialog::_onCancel(GtkWidget* w, MissionLogicDialog* self) {
    self->destroy();
}

} // namespace objectives
