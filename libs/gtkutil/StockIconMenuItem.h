#ifndef STOCKICONMENUITEM_H_
#define STOCKICONMENUITEM_H_

#include <string>
#include <gtkmm/label.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/box.h>
#include <gtkmm/image.h>

namespace gtkutil
{

/** Utility class representing a menu item with an icon and text. The
 * menu item consists of a stock icon from GTK, and a text label.
 */
class StockIconMenuItem :
	public Gtk::MenuItem
{
public:
	// Constructor takes the icon name and the label text.
	StockIconMenuItem(const Gtk::StockID& stockId, const std::string& text) :
		Gtk::MenuItem()
	{
		Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 4));

		hbox->pack_start(*Gtk::manage(new Gtk::Image(stockId, Gtk::ICON_SIZE_MENU)), false, false, 0);
		hbox->pack_start(*Gtk::manage(new Gtk::Label(text)), false, false, 0);

		add(*hbox);
	}
};

} // namespace gtkutil

#endif /*STOCKICONMENUITEM_H_*/
