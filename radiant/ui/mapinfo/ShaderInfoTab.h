#ifndef SHADERINFOTAB_H_
#define SHADERINFOTAB_H_

#include "map/ShaderBreakdown.h"

#include "gtkutil/menu/PopupMenu.h"
#include <gtkmm/liststore.h>

namespace Gtk
{
	class Label;
	class VBox;
	class TreeView;
	class Widget;
}

namespace ui {

class ShaderInfoTab
{
private:
	// The "master" widget
	Gtk::VBox* _widget;

	// The helper class counting the shaders in the map
	map::ShaderBreakdown _shaderBreakdown;

	Gtk::Label* _shaderCount;

	// Treemodel definition
	struct ListColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		ListColumns() { add(shader); add(faceCount); add(patchCount); }

		Gtk::TreeModelColumn<Glib::ustring> shader;
		Gtk::TreeModelColumn<int> faceCount;
		Gtk::TreeModelColumn<int> patchCount;
	};

	ListColumns _columns;

	// The treeview containing the above liststore
	Glib::RefPtr<Gtk::ListStore> _listStore;
	Gtk::TreeView* _treeView;

	// Context menu
	gtkutil::PopupMenu _popupMenu;

public:
	// Constructor
	ShaderInfoTab();

	// Use this to pack the tab into a parent container
	Gtk::Widget& getWidget();

	std::string getLabel();
	std::string getIconName();

private:
	// This is called to setup the widgets
	void construct();

	void _onSelectItems(bool select);
	bool _testSelectItems();

}; // class ShaderInfoTab

} // namespace ui

#endif /* SHADERINFOTAB_H_ */
