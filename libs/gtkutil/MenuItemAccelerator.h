#ifndef MENUITEMACCELERATOR_H_
#define MENUITEMACCELERATOR_H_

#include <string>
#include <gtkmm/menuitem.h>
#include <gtkmm/checkmenuitem.h>

namespace Gtk
{
	class Label;
	class Image;
	class HBox;
}

namespace gtkutil
{

class TextMenuItemBase
{
protected:
	Gtk::HBox* _hbox;

	// The corresponding widget
	Gtk::Label* _label;

	// The corresponding widget
	Gtk::Label* _accel;

	// The corresponding image widget
	Gtk::Image* _iconImage;

	TextMenuItemBase(const std::string& label,
					 const std::string& accelLabel,
					 const Glib::RefPtr<Gdk::Pixbuf>& icon);
public:
	// destructor
	virtual ~TextMenuItemBase() {}

	// Changes the label text of the given menu item
	void setLabel(const std::string& newLabel);

	// Changes the accelerator text of this menutem
	void setAccelerator(const std::string& newAccel);

	// Change the icon
	void setIcon(const Glib::RefPtr<Gdk::Pixbuf>& icon);
};

class TextMenuItemAccelerator :
	public TextMenuItemBase,
	public Gtk::MenuItem
{
public:
	/**
	 * Construct a menu item with the given label, accelerator and icon. The
	 * icon may be the empty string if no icon is required.
	 */
	TextMenuItemAccelerator(const std::string& label,
							const std::string& accelLabel,
							const Glib::RefPtr<Gdk::Pixbuf>& icon);

	// destructor
	virtual ~TextMenuItemAccelerator() {}
};

class TextToggleMenuItemAccelerator :
	public TextMenuItemBase,
	public Gtk::CheckMenuItem
{
public:
	/**
	 * Construct a menu item with the given label, accelerator and icon. The
	 * icon may be the empty string if no icon is required.
	 */
	TextToggleMenuItemAccelerator(const std::string& label,
								  const std::string& accelLabel,
								  const Glib::RefPtr<Gdk::Pixbuf>& icon);

	// destructor
	virtual ~TextToggleMenuItemAccelerator() {}
};

}

#endif /*MENUITEMACCELERATOR_H_*/
