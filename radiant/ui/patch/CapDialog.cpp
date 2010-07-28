#include "CapDialog.h"

#include "i18n.h"
#include "imainframe.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

namespace ui
{

namespace
{
	const char* WINDOW_TITLE = N_("Create Cap Patch");

	const char* const CAPTYPE_NAMES[eNumCapTypes] =
	{
		N_("Bevel"),
		N_("End Cap"),
		N_("Inverted Bevel"),
		N_("Inverted Endcap"),
		N_("Cylinder"),
	};
}

PatchCapDialog::PatchCapDialog() :
	gtkutil::Dialog(_(WINDOW_TITLE), GlobalMainFrame().getTopLevelWindow()),
	_radioButtonGroup(NULL)
{
	// Add a homogeneous hbox 
	GtkWidget* hbox = gtk_hbox_new(TRUE, 12);
	gtk_box_pack_start(GTK_BOX(_vbox), hbox, TRUE, TRUE, 0);

	// Add a table to the left
	GtkTable* leftTable = GTK_TABLE(gtk_table_new(3, 2, FALSE));
	gtk_table_set_row_spacings(leftTable, 12);
	gtk_table_set_col_spacings(leftTable, 6);

	addItemToTable(leftTable, "cap_bevel.png", 0, eCapBevel);
	addItemToTable(leftTable, "cap_endcap.png", 1, eCapEndCap);
	addItemToTable(leftTable, "cap_cylinder.png", 2, eCapCylinder);

	// Add a table to the right
	GtkTable* rightTable = GTK_TABLE(gtk_table_new(2, 2, FALSE));
	gtk_table_set_row_spacings(rightTable, 12);
	gtk_table_set_col_spacings(rightTable, 6);

	addItemToTable(rightTable, "cap_ibevel.png", 0, eCapIBevel);
	addItemToTable(rightTable, "cap_iendcap.png", 1, eCapIEndCap);

	gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(leftTable), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(rightTable), TRUE, TRUE, 0);
}

void PatchCapDialog::addItemToTable(GtkTable* table, const std::string& image, guint row, EPatchCap type)
{
	GtkWidget* img = gtk_image_new_from_pixbuf(GlobalUIManager().getLocalPixbuf(image)->gobj());
	gtk_table_attach(table, img, 0, 1, row, row+1, 
					 GtkAttachOptions(GTK_FILL), (GtkAttachOptions)0, 0, 0);

	// Create a new radio button for this cap type
	GtkWidget* radioButton = gtk_radio_button_new_with_label(_radioButtonGroup, _(CAPTYPE_NAMES[type]));
	_radioButtonGroup = gtk_radio_button_get_group(GTK_RADIO_BUTTON(radioButton));
	gtk_table_attach(table, radioButton, 1, 2, row, row+1, 
					 GtkAttachOptions(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)0, 0, 0);

	// Store the widget in the local map
	_radioButtons[type] = radioButton;
}

EPatchCap PatchCapDialog::getSelectedCapType()
{
	if (_result != RESULT_OK) return eNumCapTypes;

	for (RadioButtons::const_iterator i = _radioButtons.begin(); 
		 i != _radioButtons.end(); ++i)
	{
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(i->second)))
		{
			return i->first;
		}
	}

	return eNumCapTypes; // invalid
}

} // namespace ui
