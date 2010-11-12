#ifndef ENTITYINFOTAB_H_
#define ENTITYINFOTAB_H_

#include "map/EntityBreakdown.h"

#include <gtkmm/liststore.h>

namespace Gtk
{
	class Label;
	class VBox;
	class TreeView;
	class Widget;
}

namespace ui
{

class EntityInfoTab
{
private:
	// The "master" widget
	Gtk::VBox* _widget;

	// The helper class counting the entities in the map
	map::EntityBreakdown _entityBreakdown;

	Gtk::Label* _brushCount;
	Gtk::Label* _patchCount;
	Gtk::Label* _entityCount;

	// Treemodel definition
	struct ListColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		ListColumns() { add(eclass); add(count); }

		Gtk::TreeModelColumn<Glib::ustring> eclass;
		Gtk::TreeModelColumn<int> count;
	};

	ListColumns _columns;

	// The treeview containing the above liststore
	Glib::RefPtr<Gtk::ListStore> _listStore;
	Gtk::TreeView* _treeView;

public:
	// Constructor
	EntityInfoTab();

	// Use this to pack the tab into a parent container
	Gtk::Widget& getWidget();

	std::string getLabel();
	std::string getIconName();

private:
	// This is called to create the widgets
	void populateTab();

}; // class EntityInfoTab

} // namespace ui

#endif /* ENTITYINFOTAB_H_ */
