#ifndef MODELINFOTAB_H_
#define MODELINFOTAB_H_

#include "map/ModelBreakdown.h"

#include <gtkmm/liststore.h>

namespace Gtk
{
	class Label;
	class VBox;
	class TreeView;
	class Widget;
}

namespace ui {

class ModelInfoTab
{
private:
	// The "master" widget
	Gtk::VBox* _widget;

	// The helper class counting the models in the map
	map::ModelBreakdown _modelBreakdown;

	Gtk::Label* _modelCount;
	Gtk::Label* _skinCount;

	// Treemodel definition
	struct ListColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		ListColumns() { add(model); add(modelcount); add(polycount); add(skincount); }

		Gtk::TreeModelColumn<Glib::ustring> model;
		Gtk::TreeModelColumn<int> modelcount;
		Gtk::TreeModelColumn<int> polycount;
		Gtk::TreeModelColumn<int> skincount;
	};

	ListColumns _columns;

	// The treeview containing the above liststore
	Glib::RefPtr<Gtk::ListStore> _listStore;
	Gtk::TreeView* _treeView;

public:
	// Constructor
	ModelInfoTab();

	// Use this to pack the tab into a parent container
	Gtk::Widget& getWidget();

	std::string getLabel();
	std::string getIconName();

private:
	// This is called to create the widgets
	void populateTab();

}; // class ModelInfoTab

} // namespace ui

#endif /* MODELINFOTAB_H_ */
