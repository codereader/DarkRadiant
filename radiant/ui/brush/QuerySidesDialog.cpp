#include "QuerySidesDialog.h"

#include "iradiant.h"

#include <gtk/gtkvbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkstock.h>

#include "gtkutil/LeftAlignedLabel.h"

namespace ui
{
	namespace
	{
		const std::string WINDOW_TITLE = "Enter Number of Sides";
	}

QuerySidesDialog::QuerySidesDialog(int numSidesMin, int numSidesMax) :
	BlockingTransientWindow(WINDOW_TITLE, GlobalRadiant().getMainWindow()),
	_entry(NULL),
	_result(NUM_RESULTS),
	_numSides(-1),
	_numSidesMin(numSidesMin),
	_numSidesMax(numSidesMax)
{
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);
	
	// Create all the widgets
	populateWindow();
}

int QuerySidesDialog::queryNumberOfSides()
{
	// Enter main loop
	show();

	return (_result == RESULT_OK) ? _numSides : -1;
}

void QuerySidesDialog::populateWindow()
{
	// Create the vbox containing the notebook and the buttons
	GtkWidget* dialogVBox = gtk_vbox_new(FALSE, 6);

	// Create the spin button
	_entry = gtk_spin_button_new_with_range(_numSidesMin, _numSidesMax, 1);

	GtkWidget* entryRow = gtk_hbox_new(FALSE, 6);
	GtkWidget* label = gtkutil::LeftAlignedLabel("Number of sides: ");

	gtk_box_pack_start(GTK_BOX(entryRow), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(entryRow), _entry, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(dialogVBox), entryRow, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(dialogVBox), createButtons(), FALSE, FALSE, 0);

	// Add vbox to dialog window
	gtk_container_add(GTK_CONTAINER(getWindow()), dialogVBox);
}

GtkWidget* QuerySidesDialog::createButtons()
{
	GtkWidget* hbox = gtk_hbox_new(FALSE, 6);

	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(onOK), this);

	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(onCancel), this);

	gtk_box_pack_end(GTK_BOX(hbox), okButton, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), cancelButton, FALSE, FALSE, 0);

	return hbox;
}

void QuerySidesDialog::onCancel(GtkWidget* widget, QuerySidesDialog* self)
{
	self->_result = RESULT_CANCEL;
	self->destroy();
}

void QuerySidesDialog::onOK(GtkWidget* widget, QuerySidesDialog* self)
{
	self->_result = RESULT_OK;
	self->_numSides = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(self->_entry));
	self->destroy();
}

} // namespace ui
