#ifndef TWOCOLUMNTEXTCOMBO_H_
#define TWOCOLUMNTEXTCOMBO_H_

#include <gtkmm/combobox.h>
#include <gtkmm/liststore.h>

namespace objectives
{

namespace util
{

/**
 * \namespace objectives::util
 * Helper classes for the Objectives Editor plugin.
 *
 * \ingroup objectives
 */

/**
 * Helper class to create a Gtk::ComboBox containing two text columns.
 *
 * This class provides a convenient mechanism to create a Gtk::ComboBox backed
 * by a Gtk::ListStore containing two text columns. The first text column (column
 * 0) contains a text string which will be displayed in the Gtk::ComboBox itself,
 * whereas the second (column 1) contains a string which will not be displayed
 * and is intended for use as a code-level identifier which is not visible to
 * the user.
 */
class TwoColumnTextCombo :
	public Gtk::ComboBox
{
private:
	struct ListColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		ListColumns() { add(first); add(second); }

		Gtk::TreeModelColumn<Glib::ustring> first;
		Gtk::TreeModelColumn<Glib::ustring> second;
	};

public:
	/**
	 * Construct a TwoColumnTextCombo.
	 */
	TwoColumnTextCombo() :
		Gtk::ComboBox()
	{
		// List store and combo box
		ListColumns columns;
		Glib::RefPtr<Gtk::ListStore> ls = Gtk::ListStore::create(columns);

		set_model(ls);

		// Add a text cell renderer for column 0
		Gtk::CellRendererText* rend = Gtk::manage(new Gtk::CellRendererText);

		pack_start(*rend, false);
		add_attribute(rend->property_text(), columns.first);
	}
};

}

}

#endif /*TWOCOLUMNTEXTCOMBO_H_*/
