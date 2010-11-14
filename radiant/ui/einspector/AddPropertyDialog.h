#ifndef ADDPROPERTYDIALOG_H_
#define ADDPROPERTYDIALOG_H_

#include "gtkutil/window/BlockingTransientWindow.h"
#include "gtkutil/GladeWidgetHolder.h"

#include <string>
#include "ientity.h"

#include <gtkmm/treestore.h>
#include <gtkmm/treeselection.h>

namespace Gtk
{
	class TreeView;
	class TextView;
}

namespace ui
{

/** Modal dialog to display a list of known properties and allow the user
 * to choose one. The dialog is displayed via a single static method which
 * creates the dialog, blocks in a recursive main loop until the choice is
 * made, and then returns the string property that was selected.
 */
class AddPropertyDialog :
	public gtkutil::BlockingTransientWindow,
    private gtkutil::GladeWidgetHolder
{
public:
	typedef std::vector<std::string> PropertyList;

	// Treemodel definition
	struct TreeColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		TreeColumns()
		{
			add(displayName);
			add(propertyName);
			add(icon);
			add(description);
		}

		Gtk::TreeModelColumn<Glib::ustring> displayName;
		Gtk::TreeModelColumn<Glib::ustring> propertyName;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > icon;
		Gtk::TreeModelColumn<Glib::ustring> description;
	};

private:
	// Tree view, selection and model
	TreeColumns _columns;
	Glib::RefPtr<Gtk::TreeStore> _treeStore;
	Glib::RefPtr<Gtk::TreeSelection> _selection;

	// The selected properties
	PropertyList _selectedProperties;

	// Target entity to query for existing spawnargs
	Entity* _entity;

private:

    // Set up tree model and columns
    void setupTreeView();

	// Populate tree view with properties
	void populateTreeView();

	void updateUsagePanel();

	void _onOK();
	void _onCancel();
	void _onSelectionChanged();

	/* Private constructor creates the dialog widgets. Accepts an Entity
	 * to use for populating class-specific keys.
	 */
	AddPropertyDialog(Entity* entity);

protected:
	// Override TransientWindow::_onDeleteEvent
	void _onDeleteEvent();

public:

	/**
	 * Static method to display an AddPropertyDialog and return the chosen
	 * property.
	 *
	 * @param entity
	 * The Entity to be queried for spawnargs.
	 *
	 * @returns
	 * String list of the chosen properties (e.g. "light_radius").
	 */
	static PropertyList chooseProperty(Entity* entity);
};

} // namespace

#endif /*ADDPROPERTYDIALOG_H_*/
