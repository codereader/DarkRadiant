#include "ModalProgressDialog.h"
#include "RightAlignment.h"

#include "i18n.h"
#include "imainframe.h"

#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/button.h>
#include <gtkmm/main.h>
#include <gtkmm/stock.h>
#include <gtkmm/progressbar.h>

namespace gtkutil {

// Main constructor
ModalProgressDialog::ModalProgressDialog(const Glib::RefPtr<Gtk::Window>& parent, const std::string& title)
: gtkutil::TransientWindow(title, GlobalMainFrame().getTopLevelWindow()),
  _label(Gtk::manage(new Gtk::Label)),
  _progressBar(Gtk::manage(new Gtk::ProgressBar)),
  _aborted(false)
{
  	// Window properties
	set_modal(true);
	set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
	set_default_size(360, 80);
	set_border_width(12);

	// Create a vbox
	Gtk::VBox* vbx = Gtk::manage(new Gtk::VBox(false, 12));

	// Pack a progress bar into the window
	vbx->pack_start(*_progressBar, false, false, 0);

	// Pack the label into the window
	vbx->pack_start(*_label, true, false, 0);
	add(*vbx);

	// Pack a right-aligned cancel button at the bottom
	Gtk::Button* cancelButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));
	cancelButton->signal_clicked().connect(sigc::mem_fun(*this, &ModalProgressDialog::_onCancel));

	vbx->pack_end(*Gtk::manage(new gtkutil::RightAlignment(*cancelButton)), false, false, 0);

	// Connect the realize signal to remove the window decorations
	signal_realize().connect(sigc::mem_fun(*this, &ModalProgressDialog::_onRealize));

	// Show the window
	show_all();
	handleEvents();
}

void ModalProgressDialog::_onDeleteEvent()
{
	// Do nothing, don't call base class either
}

void ModalProgressDialog::_onRealize()
{
	// Disable some decorations
	get_window()->set_decorations(Gdk::DECOR_ALL|Gdk::DECOR_MENU|Gdk::DECOR_MINIMIZE|Gdk::DECOR_MAXIMIZE);
}

// Set the label text
void ModalProgressDialog::setText(const std::string& text)
{
	// If the aborted flag is set, throw an exception here
	if (_aborted)
	{
		throw OperationAbortedException(_("Operation cancelled by user"));
	}

	// Set the text
	_label->set_markup(text);

	// Pulse the progress bar
	_progressBar->pulse();

	// Handle GTK events to make changes visible
	handleEvents();
}

// Set the label text
void ModalProgressDialog::setTextAndFraction(const std::string& text, double fraction)
{
	// If the aborted flag is set, throw an exception here
	if (_aborted)
	{
		throw OperationAbortedException(_("Operation cancelled by user"));
	}

	// Set the text
	_label->set_markup(text);

	if (fraction < 0) 
	{
		fraction = 0.0;
	}
	else if (fraction > 1.0)
	{
		fraction = 1.0;
	}

	// Pulse the progress bar
	_progressBar->set_fraction(fraction);

	// Handle GTK events to make changes visible
	handleEvents();
}

// Handle GTK events
void ModalProgressDialog::handleEvents()
{
	while (Gtk::Main::events_pending())
	{
		Gtk::Main::iteration();
	}
}

// Cancel button callback
void ModalProgressDialog::_onCancel()
{
	_aborted = true;
}

} // namespace gtkutil
