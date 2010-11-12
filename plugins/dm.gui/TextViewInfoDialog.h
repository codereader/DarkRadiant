#ifndef TEXT_VIEW_INFO_DIALOG_H
#define TEXT_VIEW_INFO_DIALOG_H

#include "gtkutil/window/BlockingTransientWindow.h"
#include "gtkutil/ScrolledFrame.h"

#include <string.h>
#include "imainframe.h"

#include <gtkmm/textview.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/alignment.h>
#include <gtkmm/box.h>

namespace ui
{

///////////////////////////// TextViewInfoDialog:
// Small Info-Dialog showing text in a scrolled, non-editable textview and an ok button.
class TextViewInfoDialog :
	public gtkutil::BlockingTransientWindow
{
private:
	Glib::RefPtr<Gtk::TextBuffer> _bfr;

public:
	TextViewInfoDialog(const std::string& title, const std::string& text,
					   const Glib::RefPtr<Gtk::Window>& parent = Glib::RefPtr<Gtk::Window>(),
					   int win_width = 650, int win_height = 500) :
		gtkutil::BlockingTransientWindow(title, parent ? parent : GlobalMainFrame().getTopLevelWindow())
	{
		// Set the default border width in accordance to the HIG
		set_border_width(12);
		set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
		set_default_size(win_width, win_height);

		// Create the textview and add the text.
		Gtk::TextView* textView = Gtk::manage(new Gtk::TextView);
		textView->set_editable(false);
		_bfr = textView->get_buffer();

		_bfr->set_text(text);

		// Create the button and connect the signal
		Gtk::Button* okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
		okButton->signal_clicked().connect(sigc::mem_fun(*this, &TextViewInfoDialog::onOk));

		Gtk::Alignment* alignment = Gtk::manage(new Gtk::Alignment(0.5, 1, 0, 0));
		alignment->add(*okButton);

		// Create a vbox and add the elements.
		Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 6));
		vbox->pack_start(*Gtk::manage(new gtkutil::ScrolledFrame(*textView)), true, true, 0);
		vbox->pack_start(*alignment, false, false, 0);

		add(*vbox);
	}

	void onOk()
	{
		destroy();
	}
};

} // namespace

#endif /* TEXT_VIEW_INFO_DIALOG_H */
