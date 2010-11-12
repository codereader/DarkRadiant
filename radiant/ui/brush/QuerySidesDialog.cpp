#include "QuerySidesDialog.h"

#include "i18n.h"
#include "imainframe.h"

#include <gtkmm/box.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>

#include "gtkutil/LeftAlignedLabel.h"

namespace ui
{
	namespace
	{
		const char* const WINDOW_TITLE = N_("Enter Number of Sides");
	}

QuerySidesDialog::QuerySidesDialog(int numSidesMin, int numSidesMax) :
	BlockingTransientWindow(_(WINDOW_TITLE), GlobalMainFrame().getTopLevelWindow()),
	_entry(NULL),
	_result(NUM_RESULTS),
	_numSides(-1),
	_numSidesMin(numSidesMin),
	_numSidesMax(numSidesMax)
{
	set_border_width(12);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

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
	Gtk::VBox* dialogVBox = Gtk::manage(new Gtk::VBox(false, 6));

	// Create the spin button
	Gtk::Adjustment* adjustment = Gtk::manage(new Gtk::Adjustment(_numSidesMin, _numSidesMin, _numSidesMax));
	_entry = Gtk::manage(new Gtk::SpinButton(*adjustment));

	Gtk::HBox* entryRow = Gtk::manage(new Gtk::HBox(false, 6));
	Gtk::Label* label = Gtk::manage(new gtkutil::LeftAlignedLabel(_("Number of sides: ")));

	entryRow->pack_start(*label, false, false, 0);
	entryRow->pack_start(*_entry, true, true, 0);

	dialogVBox->pack_start(*entryRow, false, false, 0);
	dialogVBox->pack_start(createButtons(), false, false, 0);

	// Add vbox to dialog window
	add(*dialogVBox);
}

Gtk::Widget& QuerySidesDialog::createButtons()
{
	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 6));

	Gtk::Button* okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
	okButton->signal_clicked().connect(sigc::mem_fun(*this, &QuerySidesDialog::onOK));

	Gtk::Button* cancelButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));
	cancelButton->signal_clicked().connect(sigc::mem_fun(*this, &QuerySidesDialog::onCancel));

	hbox->pack_end(*okButton, false, false, 0);
	hbox->pack_end(*cancelButton, false, false, 0);

	return *hbox;
}

void QuerySidesDialog::onCancel()
{
	_result = RESULT_CANCEL;
	destroy();
}

void QuerySidesDialog::onOK()
{
	_result = RESULT_OK;
	_numSides = _entry->get_value_as_int();
	destroy();
}

} // namespace ui
