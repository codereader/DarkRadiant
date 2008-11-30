#include "ConversationEditor.h"

#include "iradiant.h"
#include "gtkutil/LeftalignedLabel.h"
#include "gtkutil/RightAlignment.h"
#include <gtk/gtk.h>

namespace ui {

namespace {
	const std::string WINDOW_TITLE = "Edit Conversation";

	enum {
		WIDGET_CONV_NAME_ENTRY,
	};
}

ConversationEditor::ConversationEditor(conversation::Conversation& conversation) :
	gtkutil::BlockingTransientWindow(WINDOW_TITLE, GlobalRadiant().getMainWindow()),
	_actorStore(gtk_list_store_new(2, 
								   G_TYPE_STRING,	// display name
								   G_TYPE_INT)),	// actor number
   _conversation(conversation)
{
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);

	// Create the widgets
	populateWindow();

	// Load the conversation values into the widgets
	populateWidgets();

	// Show and block
	show();
}

void ConversationEditor::populateWindow() {
	// Create the overall vbox
	GtkWidget* vbox = gtk_vbox_new(FALSE, 6);

	// Create the conversation properties pane
	gtk_box_pack_start(GTK_BOX(vbox), createPropertyPane(), FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(getWindow()), vbox);
}

GtkWidget* ConversationEditor::createPropertyPane() {
	GtkWidget* vbox = gtk_vbox_new(FALSE, 6);

	// Table for entry boxes
	GtkWidget* table = gtk_table_new(4, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_table_set_col_spacings(GTK_TABLE(table), 12);

	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

	int row = 0;
	
	// Objective description
	gtk_table_attach(GTK_TABLE(table), 
					 gtkutil::LeftAlignedLabel("<b>Name</b>"),
					 0, 1, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
	_widgets[WIDGET_CONV_NAME_ENTRY] = gtk_entry_new();
	gtk_table_attach_defaults(GTK_TABLE(table), 
							  _widgets[WIDGET_CONV_NAME_ENTRY], 
							  1, 2, row, row+1);
	
	row++;

	return vbox;
}

GtkWidget* ConversationEditor::createButtonPanel() {
	GtkWidget* buttonHBox = gtk_hbox_new(TRUE, 12);
	
	// Save button
	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(onSave), this);
	gtk_box_pack_end(GTK_BOX(buttonHBox), okButton, TRUE, TRUE, 0);
	
	// Cancel Button
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(onCancel), this);
	gtk_box_pack_end(GTK_BOX(buttonHBox), cancelButton, TRUE, TRUE, 0);
	
	return gtkutil::RightAlignment(buttonHBox);	
}

void ConversationEditor::populateWidgets() {
	// Name
	gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_CONV_NAME_ENTRY]), _conversation.name.c_str());
}

void ConversationEditor::save() {
	// TODO
}

void ConversationEditor::onSave(GtkWidget* button, ConversationEditor* self) {
	// First, save to the conversation object
	self->save();

	// Then close the window
	self->destroy();
}

void ConversationEditor::onCancel(GtkWidget* button, ConversationEditor* self) {
	// Just close the window without writing the values
	self->destroy();
}

} // namespace ui
