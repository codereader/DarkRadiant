#ifndef ICONTEXTCOLUMN_H_
#define ICONTEXTCOLUMN_H_

#include <gtkmm/treeview.h>
#include <gtkmm/treeviewcolumn.h>

namespace gtkutil
{

/**
 * A TreeViewColumn which contains an icon and a text value, the contents of
 * which are both retrieved from specified columns in the tree model.
 */
class IconTextColumn :
	public Gtk::TreeViewColumn
{
public:

	/**
	 * Construct an IconTextColumn with values retrieved from the specified
	 * columns.
	 *
	 * @param title
	 * The text title of the column.
	 *
	 * @param textCol
	 * TreeModel column containing text to display.
	 *
	 * @param iconCol
	 * TreeModel column containing the icon.
	 *
	 * @param useMarkup
	 * Whether Pango markup should be used to set the text for the column.
	 */
	IconTextColumn(const std::string& title,
				     int textCol,
				     int iconCol,
					 bool useMarkup = false) :
		Gtk::TreeViewColumn(title)
	{
		set_spacing(3);

		// Add the renderers for icon and text
		Gtk::CellRendererPixbuf* pixRend = Gtk::manage(new Gtk::CellRendererPixbuf);
		Gtk::CellRendererText* textRend = Gtk::manage(new Gtk::CellRendererText);

		pack_start(*pixRend, false);
		pack_start(*textRend, false);

		add_attribute(*pixRend, "pixbuf", iconCol);
		add_attribute(*textRend, useMarkup ? "markup" : "text", textCol);
	}

	/**
	 * Construct an IconTextColumn with values retrieved from the specified
	 * columns.
	 *
	 * @param title
	 * The text title of the column.
	 *
	 * @param textCol
	 * TreeModel column with the text to display.
	 *
	 * @param iconCol
	 * TreeModel column containing the icon.
	 *
	 * @param useMarkup
	 * Whether Pango markup should be used to set the text for the column.
	 */
	IconTextColumn(const std::string& title,
				     const Gtk::TreeModelColumn<Glib::ustring>& textCol,
					 const Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> >& iconCol,
					 bool useMarkup = false) :
		Gtk::TreeViewColumn(title)
	{
		set_spacing(3);

		pack_start(iconCol, false);
		pack_start(textCol, false);

		if (useMarkup)
		{
			Glib::ListHandle<Gtk::CellRenderer*> renderers = get_cell_renderers();

			for (Glib::ListHandle<Gtk::CellRenderer*>::iterator i = renderers.begin();
				 i != renderers.end(); ++i)
			{
				// Find the text renderer and set the markup property
				Gtk::CellRendererText* renderer = dynamic_cast<Gtk::CellRendererText*>(*i);

				if (renderer != NULL)
				{
					clear_attributes(*renderer);
					add_attribute(renderer->property_markup(), textCol);
					break;
				}
			}
		}
	}

	/**
	 * Construct an IconTextColumn with values retrieved from the specified
	 * columns.
	 *
	 * @param title
	 * The text title of the column.
	 *
	 * @param textCol
	 * TreeModel column with the text to display.
	 *
	 * @param iconCol
	 * TreeModel column containing the icon.
	 *
	 * @param useMarkup
	 * Whether Pango markup should be used to set the text for the column.
	 */
	IconTextColumn(const std::string& title,
				     const Gtk::TreeModelColumn<std::string>& textCol,
					 const Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> >& iconCol,
					 bool useMarkup = false) :
		Gtk::TreeViewColumn(title)
	{
		set_spacing(3);

		pack_start(iconCol, false);
		pack_start(textCol, false);

		if (useMarkup)
		{
			Glib::ListHandle<Gtk::CellRenderer*> renderers = get_cell_renderers();

			for (Glib::ListHandle<Gtk::CellRenderer*>::iterator i = renderers.begin();
				 i != renderers.end(); ++i)
			{
				// Find the text renderer and set the markup property
				Gtk::CellRendererText* renderer = dynamic_cast<Gtk::CellRendererText*>(*i);

				if (renderer != NULL)
				{
					clear_attributes(*renderer);
					add_attribute(renderer->property_markup(), textCol);
					break;
				}
			}
		}
	}
};

}

#endif /*ICONTEXTCOLUMN_H_*/
