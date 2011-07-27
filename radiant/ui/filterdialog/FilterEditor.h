#ifndef _FILTER_EDITOR_H_
#define _FILTER_EDITOR_H_

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

/**
 * greebo: UI for editing a single filter (name and criteria)
 */
class FilterEditor :
	public gtkutil::BlockingTransientWindow
{
public:
	enum Result
	{
		RESULT_CANCEL,
		RESULT_OK,
		NUM_RESULTS
	};

private:
	// The actual filter object to be edited
	Filter& _originalFilter;

	// The working copy of the Filter
	Filter _filter;

	std::map<int, Gtk::Widget*> _widgets;

	// Treemodel definition
	struct ListColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		ListColumns()
		{
			add(index);
			add(type);
			add(typeString);
			add(entityKey);
			add(regexMatch);
			add(showHide);
		}

		Gtk::TreeModelColumn<int> index;
		Gtk::TreeModelColumn<int> type;
		Gtk::TreeModelColumn<Glib::ustring> typeString;
		Gtk::TreeModelColumn<Glib::ustring> entityKey;
		Gtk::TreeModelColumn<Glib::ustring> regexMatch;
		Gtk::TreeModelColumn<Glib::ustring> showHide;
	};

	ListColumns _columns;

	Glib::RefPtr<Gtk::ListStore> _ruleStore;
	Gtk::TreeView* _ruleView;

	// Treemodel definition
	struct TypeStoreColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		TypeStoreColumns() { add(type); add(typeString); }

		Gtk::TreeModelColumn<int> type;
		Gtk::TreeModelColumn<Glib::ustring> typeString;
	};

	TypeStoreColumns _typeStoreColumns;
	Glib::RefPtr<Gtk::ListStore> _typeStore;

	// Treemodel definition
	struct ActionStoreColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		ActionStoreColumns() { add(boolean); add(action); }

		Gtk::TreeModelColumn<bool> boolean;
		Gtk::TreeModelColumn<Glib::ustring> action;
	};

	ActionStoreColumns _actionStoreColumns;
	Glib::RefPtr<Gtk::ListStore> _actionStore;

	int _selectedRule;

	Result _result;

	bool _updateActive;
	bool _viewOnly;

public:
	// Constructor, pass the Filter object to be edited
	FilterEditor(Filter& filter, const Glib::RefPtr<Gtk::Window>& parent, bool viewOnly);

	// Returns the dialog result (OK, CANCEL)
	Result getResult();

private:
	void populateWindow();

	void save();

	void update();
	void updateWidgetSensitivity();

	Gtk::Widget& createButtonPanel();
	Gtk::Widget& createCriteriaPanel();

	const Glib::RefPtr<Gtk::ListStore>& createTypeStore();
	const Glib::RefPtr<Gtk::ListStore>& createActionStore();

	// Converts the given enum into a string "entityclass", "object"
	std::string getStringForType(const FilterRule::Type type);
	FilterRule::Type getTypeForString(const std::string& typeStr);

	void onSave();
	void onCancel();

	void onAddRule();
	void onMoveRuleUp();
	void onMoveRuleDown();
	void onDeleteRule();

	void onRegexEdited(const Glib::ustring& path, const Glib::ustring& new_text);
	void onEntityKeyEdited(const Glib::ustring& path, const Glib::ustring& new_text);
	void onTypeEdited(const Glib::ustring& path, const Glib::ustring& new_text);
	void onActionEdited(const Glib::ustring& path, const Glib::ustring& new_text);
	void onNameEdited();

	void onRuleSelectionChanged();
};

} // namespace

#endif /* _FILTER_EDITOR_H_ */
