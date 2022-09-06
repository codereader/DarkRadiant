#pragma once

#include "wxutil/Icon.h"
#include "wxutil/dialog/DialogBase.h"
#include "wxutil/dataview/VFSTreePopulator.h"
#include "wxutil/dataview/TreeView.h"

class wxNotebook;
class wxBookCtrlEvent;

namespace ui
{

// Forward decl.
class ReadableEditorDialog;

///////////////////////////// XDataSelector:
// Selector-Dialog for TwoSided and OneSided readable guis. Switching the pages of the notebook
// also toggles the according editing mode on the ReadableEditorDialog (TwoSided or OneSided).
// Selecting a gui definition updates the guiView for previewing.
class GuiSelector :
	public wxutil::DialogBase,
	public wxutil::VFSTreePopulator::Visitor
{
public:
	// Treestore enum
	struct GuiTreeModelColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		GuiTreeModelColumns() :
			name(add(wxutil::TreeModel::Column::IconText)),
			fullName(add(wxutil::TreeModel::Column::String)),
			isFolder(add(wxutil::TreeModel::Column::Boolean))
		{}

		wxutil::TreeModel::Column name;
		wxutil::TreeModel::Column fullName;
		wxutil::TreeModel::Column isFolder;
	};

private:
	// Reference to the calling ReadableEditorDialog
	ReadableEditorDialog* _editorDialog;

	// The name that was picked.
	std::string _name;

	// The notebook holding the tabs for one-sided and two-sided readables.
	wxNotebook* _notebook;

	GuiTreeModelColumns _columns;
	wxutil::TreeModel::Ptr _oneSidedStore;
	wxutil::TreeModel::Ptr _twoSidedStore;

	wxutil::TreeView* _oneSidedView;
	wxutil::TreeView* _twoSidedView;

	wxutil::Icon _guiIcon;
	wxutil::Icon _folderIcon;

public:
	// Starts the GuiSelector and returns the name of the selected GUI or an empty string if the user canceled.
	// The dialog shows the twoSided treeview if twoSided is true.
	static std::string Run(bool twoSided, ReadableEditorDialog* editorDialog);

	void visit(wxutil::TreeModel& store, wxutil::TreeModel::Row& row,
			   const std::string& path, bool isExplicit);

	// Disconnect all events when the dialog is destroyed
	virtual bool Destroy() override;

private:
	GuiSelector(bool twoSided, ReadableEditorDialog* editorDialog);

	void fillTrees();

	void populateWindow();

	void onSelectionChanged(wxDataViewEvent& ev);
	void onPageSwitch(wxBookCtrlEvent& ev);
};

} // namespace
