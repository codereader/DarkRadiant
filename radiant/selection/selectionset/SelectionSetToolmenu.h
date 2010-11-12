#ifndef _SELECTION_SET_TOOL_MENU_H_
#define _SELECTION_SET_TOOL_MENU_H_

#include "iselectionset.h"
#include <boost/shared_ptr.hpp>
#include <gtkmm/toolitem.h>
#include <gtkmm/liststore.h>

namespace Gtk
{
	class ComboBoxEntry;
	class ToolButton;
}

namespace selection
{

class SelectionSetToolmenu :
	public ISelectionSetManager::Observer,
	public Gtk::ToolItem
{
public:
	struct ListStoreColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		ListStoreColumns() { add(name); }

		Gtk::TreeModelColumn<Glib::ustring> name;
	};

private:
	ListStoreColumns _columns;

	Glib::RefPtr<Gtk::ListStore> _listStore;
	Gtk::ToolButton* _clearSetsButton;

	Gtk::ComboBoxEntry* _entry;

public:
	SelectionSetToolmenu();

	~SelectionSetToolmenu();

	// Observer implementation
	void onSelectionSetsChanged();

private:
	// Updates the available list items and widget sensitivity
	void update();

	void onSelectionChanged();
	void onEntryActivated();
	void onDeleteAllSetsClicked();
};

} // namespace selection

#endif /* _SELECTION_SET_TOOL_MENU_H_ */
