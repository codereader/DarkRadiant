#include "ReadableEditorDialog.h"

#include "ientity.h"
#include "iregistry.h"
#include "selectionlib.h"
#include "imainframe.h"
#include "gtkutil/MultiMonitor.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/FramedWidget.h"
#include "gtkutil/dialog.h"

#include <gtk/gtk.h>

#include "string/string.h"


namespace ui
{

namespace 
{
	const std::string WINDOW_TITLE("Readable Editor");

	const std::string RKEY_READABLE_BASECLASS("game/readables/readableBaseClass");

	const char* const NO_ENTITY_ERROR = "Cannot run Readable Editor on this selection.\n"
		"Please select a single readable entity."; 

	// Widget handles for use in the _widgets std::map
	enum
	{
		WIDGET_EDIT_PANE,
		WIDGET_READABLE_NAME,
		WIDGET_XDATA_NAME,
		WIDGET_SAVEBUTTON,
	};
}

ReadableEditorDialog::ReadableEditorDialog(Entity* entity) :
	gtkutil::BlockingTransientWindow(WINDOW_TITLE, GlobalMainFrame().getTopLevelWindow()),
	_guiView(new gui::GuiView),
	_result(RESULT_CANCEL),
	_entity(entity)
{
	// Set the default border width in accordance to the HIG
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);

	// Set size of the window, default size is too narrow for path entries
	/*GdkRectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(
		GlobalMainFrame().getTopLevelWindow()
	);*/

	//gtk_window_set_default_size(GTK_WINDOW(getWindow()), static_cast<gint>(rect.width*0.4f), -1);

	// Add a vbox for the dialog elements
	GtkWidget* vbox = gtk_vbox_new(FALSE, 6);

	// The vbox is split horizontally, left are the controls, right is the preview
	GtkWidget* hbox = gtk_hbox_new(FALSE, 6);

	// The hbox contains the controls
	gtk_box_pack_start(GTK_BOX(hbox), createEditPane(), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), gtkutil::FramedWidget(_guiView->getWidget()), FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), createButtonPanel(), FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(getWindow()), vbox);

	// Load the initial values from the entity
	initControlsFromEntity();
}

void ReadableEditorDialog::initControlsFromEntity()
{
	// Inv_name
	gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_READABLE_NAME]), _entity->getKeyValue("inv_name").c_str());

	// Xdata contents
	gtk_entry_set_text(GTK_ENTRY(_widgets[WIDGET_XDATA_NAME]), _entity->getKeyValue("xdata_contents").c_str());

	// TODO: Load xdata
}

void ReadableEditorDialog::save()
{
	// Name
	_entity->setKeyValue("inv_name", gtk_entry_get_text(GTK_ENTRY(_widgets[WIDGET_READABLE_NAME])));

	// Xdata contents
	_entity->setKeyValue("xdata_contents", gtk_entry_get_text(GTK_ENTRY(_widgets[WIDGET_XDATA_NAME])));

	// TODO: Save xdata
}

GtkWidget* ReadableEditorDialog::createEditPane()
{
	GtkWidget* vbox = gtk_vbox_new(FALSE, 6);

	_widgets[WIDGET_EDIT_PANE] = vbox;

	// Create the table for the widget alignment
	GtkTable* table = GTK_TABLE(gtk_table_new(2, 2, FALSE));
	gtk_table_set_row_spacings(table, 6);
	gtk_table_set_col_spacings(table, 6);
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(table), FALSE, FALSE, 0);

	int curRow = 0;
	
	// Readable Name
	GtkWidget* nameEntry = gtk_entry_new();
	_widgets[WIDGET_READABLE_NAME] = nameEntry;

	GtkWidget* nameLabel = gtkutil::LeftAlignedLabel("Inventory Name:");

	gtk_table_attach_defaults(table, nameLabel, 0, 1, curRow, curRow+1);
	gtk_table_attach_defaults(table, nameEntry, 1, 2, curRow, curRow+1);

	curRow++;

	// XData Name
	GtkWidget* xdataNameEntry = gtk_entry_new();
	_widgets[WIDGET_XDATA_NAME] = nameEntry;

	// Reserve space for 40 characters
	gtk_entry_set_width_chars(GTK_ENTRY(xdataNameEntry), 40);

	GtkWidget* xDataNameLabel = gtkutil::LeftAlignedLabel("XData Name:");

	gtk_table_attach_defaults(table, xDataNameLabel, 0, 1, curRow, curRow+1);
	gtk_table_attach_defaults(table, xdataNameEntry, 1, 2, curRow, curRow+1);

	return vbox;
}

GtkWidget* ReadableEditorDialog::createButtonPanel()
{
	GtkWidget* hbx = gtk_hbox_new(TRUE, 6);

	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	_widgets[WIDGET_SAVEBUTTON] = gtk_button_new_from_stock(GTK_STOCK_SAVE);
	
	g_signal_connect(
		G_OBJECT(cancelButton), "clicked", G_CALLBACK(onCancel), this
	);
	g_signal_connect(
		G_OBJECT(_widgets[WIDGET_SAVEBUTTON]), "clicked", G_CALLBACK(onSave), this
	);

	gtk_box_pack_end(GTK_BOX(hbx), _widgets[WIDGET_SAVEBUTTON], TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbx), cancelButton, TRUE, TRUE, 0);

	return gtkutil::RightAlignment(hbx);
}

void ReadableEditorDialog::_postShow()
{
	// Initialise the GL widget after the widgets have been shown
	_guiView->initialiseView();

	BlockingTransientWindow::_postShow();
}

void ReadableEditorDialog::RunDialog(const cmd::ArgumentList& args)
{
	// Check prerequisites
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	if (info.entityCount == 1 && info.totalCount == info.entityCount)
	{
		// Check the entity type
		Entity* entity = Node_getEntity(GlobalSelectionSystem().ultimateSelected());

		if (entity != NULL && entity->getKeyValue("editor_readable") == "1")
		{
			// Show the dialog
			ReadableEditorDialog dialog(entity);
			dialog.show();

			return;
		}
	}

	// Exactly one redable entity must be selected.
	gtkutil::errorDialog(NO_ENTITY_ERROR, GlobalMainFrame().getTopLevelWindow());
}

void ReadableEditorDialog::onCancel(GtkWidget* widget, ReadableEditorDialog* self) 
{
	self->_result = RESULT_CANCEL;

	self->destroy();
}

void ReadableEditorDialog::onSave(GtkWidget* widget, ReadableEditorDialog* self) 
{
	self->_result = RESULT_OK;

	self->save();

	// Done, just destroy the window
	self->destroy();
}

} // namespace ui
