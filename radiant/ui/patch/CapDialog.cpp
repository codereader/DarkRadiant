#include "CapDialog.h"

#include "i18n.h"
#include "imainframe.h"

#include <gtkmm/box.h>
#include <gtkmm/table.h>
#include <gtkmm/image.h>
#include <gtkmm/radiobutton.h>

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
	gtkutil::Dialog(_(WINDOW_TITLE), GlobalMainFrame().getTopLevelWindow())
{
	// Add a homogeneous hbox to the protected _vbox member
	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(true, 12));
	_vbox->pack_start(*hbox, true, true, 0);

	// Add a table to the left
	Gtk::Table* leftTable = Gtk::manage(new Gtk::Table(3, 2, false));
	leftTable->set_row_spacings(12);
	leftTable->set_col_spacings(6);

	addItemToTable(*leftTable, "cap_bevel.png", 0, eCapBevel);
	addItemToTable(*leftTable, "cap_endcap.png", 1, eCapEndCap);
	addItemToTable(*leftTable, "cap_cylinder.png", 2, eCapCylinder);

	// Add a table to the right
	Gtk::Table* rightTable = Gtk::manage(new Gtk::Table(2, 2, false));
	rightTable->set_row_spacings(12);
	rightTable->set_col_spacings(6);

	addItemToTable(*rightTable, "cap_ibevel.png", 0, eCapIBevel);
	addItemToTable(*rightTable, "cap_iendcap.png", 1, eCapIEndCap);

	hbox->pack_start(*leftTable, true, true, 0);
	hbox->pack_start(*rightTable, true, true, 0);
}

void PatchCapDialog::addItemToTable(Gtk::Table& table, const std::string& image, int row, EPatchCap type)
{
	Gtk::Image* img = Gtk::manage(new Gtk::Image(GlobalUIManager().getLocalPixbuf(image)));

	table.attach(*img, 0, 1, row, row+1, Gtk::FILL, Gtk::AttachOptions(0), 0, 0);

	// Create a new radio button for this cap type
	Gtk::RadioButton* radioButton = Gtk::manage(new Gtk::RadioButton(_group, _(CAPTYPE_NAMES[type])));
	_group = radioButton->get_group();

	table.attach(*radioButton, 1, 2, row, row+1, Gtk::FILL|Gtk::EXPAND, Gtk::AttachOptions(0), 0, 0);

	// Store the widget in the local map
	_radioButtons[type] = radioButton;
}

EPatchCap PatchCapDialog::getSelectedCapType()
{
	if (_result != RESULT_OK) return eNumCapTypes;

	for (RadioButtons::const_iterator i = _radioButtons.begin();
		 i != _radioButtons.end(); ++i)
	{
		if (i->second->get_active())
		{
			return i->first;
		}
	}

	return eNumCapTypes; // invalid
}

} // namespace ui
