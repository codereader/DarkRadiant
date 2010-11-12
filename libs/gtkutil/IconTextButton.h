#ifndef ICONTEXTBUTTON_H_
#define ICONTEXTBUTTON_H_

#include <string>
#include <gtkmm/label.h>
#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/button.h>
#include <gtkmm/togglebutton.h>
#include <gdkmm/pixbuf.h>

namespace gtkutil
{

/** Button with an icon above and a title underneath.
 */
class IconTextButton :
	public Gtk::Button
{
public:
	/** Construct an IconTextButton with the given label text and local icon
	 * path.
	 *
	 * @param name
	 * The text to display under the icon, can be empty.
	 *
	 * @param icon
	 * The icon to pack into the button.
	 */
	IconTextButton(const std::string& name,
				   const Glib::RefPtr<Gdk::Pixbuf>& icon)
	{
		// Create vbox containing image and label
		Gtk::VBox* vbx = Gtk::manage(new Gtk::VBox(false, 3));
		Gtk::Image* img = Gtk::manage(new Gtk::Image(icon));

		vbx->pack_start(*img, true, false, 0);

		if (name != "")
		{
			Gtk::Label* label = Gtk::manage(new Gtk::Label(name));
			vbx->pack_end(*label, true, false, 0);
		}

		add(*vbx);

		// Set the button to standard size
		set_size_request(3 * icon->get_width(), -1);
	}
};

class IconTextToggleButton :
	public Gtk::ToggleButton
{
public:
	/** Construct an IconTextToggleButton with the given label text and icon.
	 *
	 * @param name
	 * The text to display under the icon, can be empty.
	 *
	 * @param icon
	 * The icon to pack into the button.
	 */
	IconTextToggleButton(const std::string& name,
						 const Glib::RefPtr<Gdk::Pixbuf>& icon)
	{
		// Create vbox containing image and label
		Gtk::VBox* vbx = Gtk::manage(new Gtk::VBox(false, 3));
		Gtk::Image* img = Gtk::manage(new Gtk::Image(icon));

		vbx->pack_start(*img, true, false, 0);

		if (name != "")
		{
			Gtk::Label* label = Gtk::manage(new Gtk::Label(name));
			vbx->pack_end(*label, true, false, 0);
		}

		add(*vbx);

		// Set the button to standard size
		set_size_request(3 * icon->get_width(), -1);
	}
};

}

#endif /*ICONTEXTBUTTON_H_*/
