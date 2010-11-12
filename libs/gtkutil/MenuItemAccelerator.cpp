#include "MenuItemAccelerator.h"

#include <gtkmm/checkmenuitem.h>
#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/image.h>

namespace gtkutil
{

TextMenuItemBase::TextMenuItemBase(const std::string& label,
	const std::string& accelLabel, const Glib::RefPtr<Gdk::Pixbuf>& icon) :
	_label(NULL),
	_accel(NULL),
	_iconImage(NULL)
{
	// Create the text. This consists of the icon, the label string (left-
	// aligned) and the accelerator string (right-aligned).
	_hbox = Gtk::manage(new Gtk::HBox(false, 4));

	// Try to pack in icon ONLY if it is valid
	if (icon)
	{
		_iconImage = Gtk::manage(new Gtk::Image(icon));
		_hbox->pack_start(*_iconImage, false, false, 0);
	}

	_label = Gtk::manage(new Gtk::Label(label, true));
	_accel = Gtk::manage(new Gtk::Label(accelLabel, true));

	_hbox->pack_start(*_label, false, false, 0);
	_hbox->pack_start(*Gtk::manage(new Gtk::Label(" ")), false, false, 12);
	_hbox->pack_end(*_accel, false, false, 0);
}

void TextMenuItemBase::setLabel(const std::string& newLabel)
{
	if (_label != NULL)
	{
		_label->set_markup_with_mnemonic(newLabel);
	}
}

void TextMenuItemBase::setAccelerator(const std::string& newAccel)
{
	if (_accel != NULL)
	{
		_accel->set_markup_with_mnemonic(newAccel);
	}
}

void TextMenuItemBase::setIcon(const Glib::RefPtr<Gdk::Pixbuf>& icon)
{
	if (_iconImage != NULL)
	{
		_iconImage->set(icon);
	}
}

TextMenuItemAccelerator::TextMenuItemAccelerator(const std::string& label,
		 const std::string& accelLabel, const Glib::RefPtr<Gdk::Pixbuf>& icon)
:	TextMenuItemBase(label, accelLabel, icon),
	Gtk::MenuItem()
{
	// Pack the label structure into the MenuItem
	add(*_hbox);
}

TextToggleMenuItemAccelerator::TextToggleMenuItemAccelerator(const std::string& label,
		 const std::string& accelLabel, const Glib::RefPtr<Gdk::Pixbuf>& icon)
:	TextMenuItemBase(label, accelLabel, icon),
	Gtk::CheckMenuItem()
{
	// Pack the label structure into the MenuItem
	add(*_hbox);
}


} // namespace gtkutil
