#include "PatchThickenDialog.h"

#include "i18n.h"
#include "imainframe.h"
#include "string/convert.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"

#include <gtkmm/box.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/label.h>
#include <gtkmm/entry.h>
#include <gtkmm/table.h>

namespace
{
	const char* const WINDOW_TITLE = N_("Patch Thicken");
	const char* const LABEL_TITLE = N_("Thicken selected Patches");
	const char* const LABEL_THICKNESS_ENTRY = N_("Thickness (units):");
	const char* const LABEL_CREATE_SEAMS = N_("Create _Seams (\"side walls\")");
	const char* const LABEL_EXTRUDE_NORMALS = N_("Extrude along Vertex Normals");
	const char* const LABEL_EXTRUDE_X = N_("Extrude along X-Axis");
	const char* const LABEL_EXTRUDE_Y = N_("Extrude along Y-Axis");
	const char* const LABEL_EXTRUDE_Z = N_("Extrude along Z-Axis");

	const float DEFAULT_THICKNESS = 16.0f;
	const bool DEFAULT_CREATE_SEAMS = TRUE;
}

namespace ui
{

PatchThickenDialog::PatchThickenDialog() :
	gtkutil::Dialog(_(WINDOW_TITLE), GlobalMainFrame().getTopLevelWindow())
{
	// Create the title label (bold font)
	Gtk::Label* topLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(
		std::string("<span weight=\"bold\">") + _(LABEL_TITLE) + "</span>"
	));
    topLabel->set_padding(6, 2);

	_vbox->pack_start(*topLabel, false, false, 0);

	// Create the entry field
	Gtk::Label* thicknessLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(_(LABEL_THICKNESS_ENTRY)));

	_thicknessEntry = Gtk::manage(new Gtk::Entry);
	_thicknessEntry->set_text(string::to_string(DEFAULT_THICKNESS));

	// Create a new 2x5 table and pack it into an alignment
	Gtk::Table* table = Gtk::manage(new Gtk::Table(2, 5, false));
    table->set_col_spacings(12);
    table->set_row_spacings(6);

    // Indent the table by adding a left-padding to the alignment
    Gtk::Alignment* alignment = Gtk::manage(new gtkutil::LeftAlignment(*table, 18, 1));

    // Create the radio button group for choosing the extrude axis
	_radNormals = Gtk::manage(new Gtk::RadioButton(_group, _(LABEL_EXTRUDE_NORMALS)));
	_group = _radNormals->get_group();

	_radX = Gtk::manage(new Gtk::RadioButton(_group, _(LABEL_EXTRUDE_X)));
	_radY = Gtk::manage(new Gtk::RadioButton(_group, _(LABEL_EXTRUDE_Y)));
	_radZ = Gtk::manage(new Gtk::RadioButton(_group, _(LABEL_EXTRUDE_Z)));

    // Pack the buttons into the table
	table->attach(*_radNormals, 0, 2, 0, 1);
	table->attach(*_radX, 0, 2, 1, 2);
	table->attach(*_radY, 0, 2, 2, 3);
	table->attach(*_radZ, 0, 2, 3, 4);

    // Pack the thickness entry field into the table
    table->attach(*thicknessLabel, 0, 1, 4, 5);
    table->attach(*_thicknessEntry, 1, 2, 4, 5);

	// Create the "create seams" label
	_seamsCheckBox = Gtk::manage(new Gtk::CheckButton(_(LABEL_CREATE_SEAMS), true));
	_seamsCheckBox->set_active(DEFAULT_CREATE_SEAMS);

	table->attach(*_seamsCheckBox, 0, 2, 5, 6);

	// Pack the table into the dialog
	_vbox->pack_start(*alignment, true, true, 0);
}

float PatchThickenDialog::getThickness()
{
	return string::convert<float>(_thicknessEntry->get_text());
}

bool PatchThickenDialog::getCeateSeams()
{
	return _seamsCheckBox->get_active();
}

int PatchThickenDialog::getAxis()
{
	if (_radX->get_active())
	{
		return 0;
	}
	else if (_radY->get_active())
	{
		return 1;
	}
	else if (_radZ->get_active())
	{
		return 2;
	}
	else
	{
		// Extrude along normals
		return 3;
	}
}

} // namespace ui
