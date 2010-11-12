#ifndef _FILTER_DIALOG_H_
#define _FILTER_DIALOG_H_

#include "icommandsystem.h"
#include "gtkutil/window/BlockingTransientWindow.h"
#include <map>
#include "Filter.h"
#include <gtkmm/liststore.h>

namespace Gtk
{
	class TreeView;
	class Widget;
}

namespace ui
{

class FilterDialog :
	public gtkutil::BlockingTransientWindow
{
private:
	// Treemodel definition
	struct TreeColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		TreeColumns()
		{
			add(name);
			add(state);
			add(colour);
			add(readonly);
		}

		Gtk::TreeModelColumn<Glib::ustring> name;
		Gtk::TreeModelColumn<Glib::ustring> state;
		Gtk::TreeModelColumn<Glib::ustring> colour;
		Gtk::TreeModelColumn<bool> readonly;
	};

	TreeColumns _columns;

	Glib::RefPtr<Gtk::ListStore> _filterStore;

	// The treeview listing all the filters
	Gtk::TreeView* _filterView;

	std::map<int, Gtk::Widget*> _widgets;

	// Holds the name of the currently selected filter (or "" if none selected)
	std::string _selectedFilter;

	// The current working set, indexed by name
	typedef std::map<std::string, FilterPtr> FilterMap;
	FilterMap _filters;

	// The deleted filters, the actual deletion happens in the save() method
	FilterMap _deletedFilters;

	// Private constructor
	FilterDialog();

public:
	/**
	 * greebo: Shows the dialog (command target)
	 */
	static void showDialog(const cmd::ArgumentList& args);

private:
	// Saves filter settings and exits
	void save();

	// Reload filter settings and fill widges
	void update();

	// Loads the filters from the filtersystem (happens at dialog construction)
	void loadFilters();

	// This is called to create the widgets
	void populateWindow();

	Gtk::Widget& createButtonPanel();
	Gtk::Widget& createFiltersPanel();

	// Update buttons
	void updateWidgetSensitivity();

	// gtkmm callbacks
	void onSave();
	void onCancel();

	void onAddFilter();
	void onEditFilter();
	void onViewFilter();
	void onDeleteFilter();

	void onFilterSelectionChanged();
};

} // namespace ui

#endif /* _FILTER_DIALOG_H_ */
