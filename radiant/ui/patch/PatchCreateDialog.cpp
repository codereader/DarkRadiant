#include "PatchCreateDialog.h"

#include "i18n.h"
#include "imainframe.h"
#include "selectionlib.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "string/convert.h"

#include <gtkmm/comboboxtext.h>
#include <gtkmm/table.h>
#include <gtkmm/alignment.h>
#include <gtkmm/box.h>
#include <gtkmm/label.h>

namespace
{
	const char* WINDOW_TITLE = N_("Create Flat Patch Mesh");
	const char* LABEL_TITLE = N_("Create Simple Patch Mesh");
	const char* LABEL_WIDTH_COMBO = N_("Width: ");
	const char* LABEL_HEIGHT_COMBO = N_("Height: ");
	const char* LABEL_REMOVE_BRUSHES = N_("Remove selected Brush");

	const bool DEFAULT_REMOVE_BRUSHES = false;

	const int MIN_PATCH_DIM = 3;
	const int MAX_PATCH_DIM = 15;
	const int INCR_PATCH_DIM = 2;
}

namespace ui {

PatchCreateDialog::PatchCreateDialog() :
	gtkutil::Dialog(_(WINDOW_TITLE), GlobalMainFrame().getTopLevelWindow())
{
    // Create the title label (bold font)
	Gtk::Label* topLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(
		std::string("<span weight=\"bold\">") + _(LABEL_TITLE) + "</span>"
	));
    topLabel->set_padding(6, 2);

	_vbox->pack_start(*topLabel, false, false, 0);

    // Create the labels for the combo boxes
	Gtk::Label* labelWidth = Gtk::manage(new gtkutil::LeftAlignedLabel(_(LABEL_WIDTH_COMBO)));
	Gtk::Label* labelHeight = Gtk::manage(new gtkutil::LeftAlignedLabel(_(LABEL_HEIGHT_COMBO)));

	// Create the two combo boxes for width and height
	_comboWidth = Gtk::manage(new Gtk::ComboBoxText);
	_comboHeight = Gtk::manage(new Gtk::ComboBoxText);

	// Fill the values into the combo boxes
	for (int i = MIN_PATCH_DIM; i <= MAX_PATCH_DIM; i += INCR_PATCH_DIM)
	{
		_comboWidth->append_text(string::to_string(i));
		_comboHeight->append_text(string::to_string(i));
	}

	// Activate the first item in the combo boxes
	_comboWidth->set_active(0);
	_comboHeight->set_active(0);

	// Create a new 2x5 table and pack it into an alignment
	Gtk::Table* table = Gtk::manage(new Gtk::Table(2, 5, false));
    table->set_col_spacings(12);
    table->set_row_spacings(6);

	// Indent the table by adding a left-padding to the alignment
	Gtk::Alignment* alignment = Gtk::manage(new gtkutil::LeftAlignment(*table, 18, 1.0f));

    // Pack the widgets into the table
	table->attach(*labelWidth, 0, 1, 0, 1);
	table->attach(*_comboWidth, 1, 2, 0, 1);
	table->attach(*labelHeight, 0, 1, 1, 2);
	table->attach(*_comboHeight, 1, 2, 1, 2);

	// Create the "remove brushes" label
	_removeBrushCheckbox = Gtk::manage(new Gtk::CheckButton(_(LABEL_REMOVE_BRUSHES), true));
	_removeBrushCheckbox->set_active(DEFAULT_REMOVE_BRUSHES);

	table->attach(*_removeBrushCheckbox, 0, 2, 2, 3);

	// Pack the table into the dialog
	_vbox->pack_start(*alignment, true, true, 0);
}

void PatchCreateDialog::_postShow()
{
	// Activate/Inactivate the check box depending on the selected brush count
	if (GlobalSelectionSystem().getSelectionInfo().brushCount == 1)
	{
		_removeBrushCheckbox->set_sensitive(true);
		_removeBrushCheckbox->set_active(true);
	}
	else
	{
		_removeBrushCheckbox->set_sensitive(false);
		_removeBrushCheckbox->set_active(false);
	}

	gtkutil::Dialog::_postShow();
}

int PatchCreateDialog::getSelectedWidth()
{
	return string::convert<int>(_comboWidth->get_active_text());
}

int PatchCreateDialog::getSelectedHeight()
{
	return string::convert<int>(_comboHeight->get_active_text());
}

bool PatchCreateDialog::getRemoveSelectedBrush()
{
	return _removeBrushCheckbox->get_active();
}

} // namespace ui
