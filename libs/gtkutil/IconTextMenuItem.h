#ifndef ICONMENULABEL_H_
#define ICONMENULABEL_H_

#include <string>

#include <gtkmm/label.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/box.h>
#include <gtkmm/image.h>

namespace gtkutil
{

/**
 * Utility class representing a menu item with an icon and text. The local
 * icon image name and the label text are passed to the constructor, which
 * creates the required Widgets.
 */
class IconTextMenuItem :
	public Gtk::MenuItem
{
public:
	// Constructor takes the icon name and the label text.
	IconTextMenuItem(const Glib::RefPtr<Gdk::Pixbuf>& icon, const std::string& text)
	{
		Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 4));

		hbox->pack_start(*Gtk::manage(new Gtk::Image(icon)), false, false, 0);
		hbox->pack_start(*Gtk::manage(new Gtk::Label(text)), false, false, 0);

		add(*hbox);
	}
};

} // namespace gtkutil

#endif /*ICONMENULABEL_H_*/
