#ifndef TEXTCOLUMN_H_
#define TEXTCOLUMN_H_

#include <gtkmm/cellrenderertext.h>
#include <gtkmm/treeviewcolumn.h>

namespace gtkutil
{

/** A GtkTreeViewColumn that displays the text contained in the provided
 * column number. The text is interpreted as Pango markup to allow formatting
 * such as bold text.
 */
class TextColumn :
	public Gtk::TreeViewColumn
{
public:
	/** Create a TextColumn which displays the text in the given column.
	 *
	 * @param title
	 * The title of the column.
	 *
	 * @param textColumn
	 * The column reference to display text from.
	 *
	 * @param useMarkup
	 * Whether to use Pango markup to format text in the column (default true).
	 */
	TextColumn(const std::string& title,
				const Gtk::TreeModelColumnBase& textColumn,
				bool useMarkup = true) :
		Gtk::TreeViewColumn(title, *Gtk::manage(new Gtk::CellRendererText))
	{
		// Get the cell renderer from the column (as created in the constructor)
		Gtk::CellRendererText* renderer = static_cast<Gtk::CellRendererText*>(get_first_cell_renderer());

		// Associate the cell renderer with the given model column
		if (useMarkup)
		{
			add_attribute(renderer->property_markup(), textColumn);
		}
		else
		{
			add_attribute(renderer->property_text(), textColumn);
		}
	}

	Gtk::CellRendererText* getCellRenderer()
	{
		return static_cast<Gtk::CellRendererText*>(get_first_cell_renderer());
	}
};

class ColouredTextColumn :
	public TextColumn
{
public:
	/** Create a TextColumn which displays the text in the given column.
	 *
	 * @param title
	 * The title of the column.
	 *
	 * @param textColumn
	 * The column reference to display text from.
	 *
	 * @param colourColumn
	 * The column reference to retrieve the colour from
	 * This is a string column, containing "#707070", "black", etc.
	 *
	 * @param useMarkup
	 * Whether to use Pango markup to format text in the column (default true).
	 */
	ColouredTextColumn(const std::string& title,
					   const Gtk::TreeModelColumnBase& textColumn,
					   const Gtk::TreeModelColumn<Glib::ustring>& colourColumn,
					   bool useMarkup = true) :
		TextColumn(title, textColumn, useMarkup)
	{
		// Get the cell renderer from the column (as created in the constructor)
		Gtk::CellRendererText* renderer = static_cast<Gtk::CellRendererText*>(get_first_cell_renderer());

		// Associate foreground colour
		add_attribute(renderer->property_foreground(), colourColumn);
	}
};

}

#endif /*TEXTCOLUMN_H_*/
