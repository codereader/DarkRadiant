#ifndef AI_VOCAL_SET_CHOOSER_DIALOG_H_
#define AI_VOCAL_SET_CHOOSER_DIALOG_H_

#include "gtkutil/window/BlockingTransientWindow.h"
#include <set>
#include <map>
#include <gtkmm/liststore.h>

#include "AIVocalSetPreview.h"

namespace Gtk
{
	class TreeView;
	class Button;
	class TextView;
}

namespace ui
{

class AIVocalSetChooserDialog :
	public gtkutil::BlockingTransientWindow
{
public:
	typedef std::set<std::string> SetList;

	enum Result
	{
		RESULT_OK,
		RESULT_CANCEL,
		NUM_RESULTS,
	};

private:
	struct ListStoreColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		ListStoreColumns() { add(name); }

		Gtk::TreeModelColumn<Glib::ustring> name;
	};

	ListStoreColumns _columns;

	Glib::RefPtr<Gtk::ListStore> _setStore;
	Gtk::TreeView* _setView;

	Gtk::Button* _okButton;
	Gtk::TextView* _description;

	// The name of the currently selected set
	std::string _selectedSet;

	static SetList _availableSets;

	Result _result;

	AIVocalSetPreview* _preview;

public:
	AIVocalSetChooserDialog();

	// Set the selection to a given entityDef
	void setSelectedVocalSet(const std::string& setName);

	// Get the currently selected set (is empty when nothing is selected)
	std::string getSelectedVocalSet();

	// Get the result (whether the user clicked OK or Cancel)
	Result getResult();

private:
	void populateSetStore();

	Gtk::Widget& createButtonPanel();
	Gtk::Widget& createDescriptionPanel();

	// Searches all entity classes for available sets
	static void findAvailableSets();

	void onSetSelectionChanged();
	void onOK();
	void onCancel();
};

} // namespace ui

#endif /* AI_VOCAL_SET_CHOOSER_DIALOG_H_ */
