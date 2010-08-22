#include "MessageBox.h"

#include "itextstream.h"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gdk/gdkkeysyms.h>

#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/RightAlignment.h"

namespace gtkutil
{

MessageBox::MessageBox(const std::string& title, const std::string& text,
					   IDialog::MessageType type, const Glib::RefPtr<Gtk::Window>& parent) :
	Dialog(title, parent),
	_text(text),
	_type(type)
{}

// Constructs the dialog (adds buttons, text and icons)
void MessageBox::construct()
{
	// Base class is adding the buttons
	Dialog::construct();

	// Add an hbox for the icon and the content
	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 6));
	_vbox->pack_start(*hbox, true, true, 0);

	// Add the icon
	Gtk::Widget* icon = createIcon();

	if (icon != NULL)
	{
		hbox->pack_start(*icon, false, false, 0);
	}

	// Add the text
	hbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(_text)), true, true, 0);
}

Gtk::Widget* MessageBox::createIcon()
{
	Gtk::BuiltinStockID stockId;

	switch (_type)
	{
	case MESSAGE_CONFIRM:
		stockId = Gtk::Stock::DIALOG_INFO;
		break;
	case MESSAGE_ASK:
		stockId = Gtk::Stock::DIALOG_QUESTION;
		break;
	case MESSAGE_WARNING:
		stockId = Gtk::Stock::DIALOG_WARNING;
		break;
	case MESSAGE_ERROR:
		stockId = Gtk::Stock::DIALOG_ERROR;
		break;
	default:
		stockId = Gtk::Stock::DIALOG_INFO;
	};

	return Gtk::manage(new Gtk::Image(stockId, Gtk::ICON_SIZE_DIALOG));
}

// Override Dialog::createButtons() to add the custom ones
Gtk::Widget& MessageBox::createButtons()
{
	Gtk::HBox* buttonHBox = Gtk::manage(new Gtk::HBox(false, 6));

	if (_type == MESSAGE_CONFIRM || _type == MESSAGE_WARNING || _type == MESSAGE_ERROR)
	{
		// Add an OK button
		Gtk::Button* okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::OK));
		okButton->signal_clicked().connect(sigc::mem_fun(*this, &MessageBox::onOK));
		buttonHBox->pack_end(*okButton, false, false, 0);

		mapKeyToButton(GDK_O, *okButton);
		mapKeyToButton(GDK_Return, *okButton);
		mapKeyToButton(GDK_Escape, *okButton);
	}
	else if (_type == MESSAGE_ASK)
	{
		// YES button
		Gtk::Button* yesButton = Gtk::manage(new Gtk::Button(Gtk::Stock::YES));
		yesButton->signal_clicked().connect(sigc::mem_fun(*this, &MessageBox::onYes));
		buttonHBox->pack_end(*yesButton, false, false, 0);

		mapKeyToButton(GDK_Y, *yesButton);
		mapKeyToButton(GDK_Return, *yesButton);
		
		// NO button
		Gtk::Button* noButton = Gtk::manage(new Gtk::Button(Gtk::Stock::NO));
		noButton->signal_clicked().connect(sigc::mem_fun(*this, &MessageBox::onNo));
		buttonHBox->pack_end(*noButton, false, false, 0);

		mapKeyToButton(GDK_N, *noButton);
		mapKeyToButton(GDK_Escape, *noButton);
	}
	else if (_type == MESSAGE_YESNOCANCEL)
	{
		// YES button
		Gtk::Button* yesButton = Gtk::manage(new Gtk::Button(Gtk::Stock::YES));
		yesButton->signal_clicked().connect(sigc::mem_fun(*this, &MessageBox::onYes));
		buttonHBox->pack_start(*yesButton, false, false, 0);

		mapKeyToButton(GDK_Y, *yesButton);
		mapKeyToButton(GDK_Return, *yesButton);
		
		// NO button
		Gtk::Button* noButton = Gtk::manage(new Gtk::Button(Gtk::Stock::NO));
		noButton->signal_clicked().connect(sigc::mem_fun(*this, &MessageBox::onNo));
		buttonHBox->pack_start(*noButton, false, false, 0);

		mapKeyToButton(GDK_N, *noButton);

		// Cancel button
		Gtk::Button* cancelButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CANCEL));
		cancelButton->signal_clicked().connect(sigc::mem_fun(*this, &MessageBox::onCancel));
		buttonHBox->pack_start(*cancelButton, false, false, 0);

		mapKeyToButton(GDK_Escape, *cancelButton);
		mapKeyToButton(GDK_C, *cancelButton);
	}
	else
	{
		globalErrorStream() << "Invalid message type encountered: " << _type << std::endl;
	}
	
	return *Gtk::manage(new RightAlignmentmm(*buttonHBox));
}

void MessageBox::onYes()
{
	_result = ui::IDialog::RESULT_YES;
	hide(); // breaks gtk_main()
}

void MessageBox::onNo()
{
	_result = ui::IDialog::RESULT_NO;
	hide(); // breaks gtk_main()
}

} // namespace gtkutil
