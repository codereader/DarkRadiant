#pragma once

#include "gtkutil/window/BlockingTransientWindow.h"
#include "gtkutil/preview/ModelPreview.h"

#include <set>
#include <map>
#include <gtkmm/liststore.h>

namespace Gtk
{
	class TreeView;
	class Button;
	class TextView;
}

namespace ui
{

class AIHeadChooserDialog :
	public gtkutil::BlockingTransientWindow
{
public:
	typedef std::set<std::string> HeadList;

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
	Glib::RefPtr<Gtk::ListStore> _headStore;
	Gtk::TreeView* _headsView;

	Gtk::Button* _okButton;
	Gtk::TextView* _description;

	// The model preview
    gtkutil::ModelPreviewPtr _preview;

	// The name of the currently selected head
	std::string _selectedHead;

	static HeadList _availableHeads;

	Result _result;

public:
	AIHeadChooserDialog();

	// Set the selection to a given entityDef
	void setSelectedHead(const std::string& headDef);

	// Get the currently selected head (is empty when nothing is selected)
	std::string getSelectedHead();

	// Get the result (whether the user clicked OK or Cancel)
	Result getResult();

private:
	// Override base class
	void _postShow();

	void populateHeadStore();

	Gtk::Widget& createButtonPanel();
	Gtk::Widget& createDescriptionPanel();

	// Searches all entity classes for available heads
	static void findAvailableHeads();

	void onHeadSelectionChanged();
	void onOK();
	void onCancel();
};

} // namespace ui
