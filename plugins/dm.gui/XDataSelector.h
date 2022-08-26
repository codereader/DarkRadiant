#pragma once

#include "XDataLoader.h"
#include "wxutil/Icon.h"
#include "wxutil/dialog/DialogBase.h"
#include "wxutil/dataview/VFSTreePopulator.h"
#include "wxutil/dataview/TreeView.h"

namespace ui
{

class ReadableEditorDialog;

///////////////////////////// XDataSelector:
// Runs a dialog for choosing XData definitions, which updates the guiView of the calling
// ReadableEditorDialog for previewing.
class XDataSelector :
	public wxutil::DialogBase,
	public wxutil::VFSTreePopulator::Visitor
{
private:
	// Treestore enum
	struct XdataTreeModelColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		XdataTreeModelColumns() :
			name(add(wxutil::TreeModel::Column::IconText)),
			fullName(add(wxutil::TreeModel::Column::String)),
			isFolder(add(wxutil::TreeModel::Column::Boolean))
		{}

		wxutil::TreeModel::Column name;
		wxutil::TreeModel::Column fullName;
		wxutil::TreeModel::Column isFolder;
	};

	// The tree
	XdataTreeModelColumns _columns;
	wxutil::TreeModel::Ptr _store;
	wxutil::TreeView* _view;

	// A Map of XData files. Basically just the keyvalues are needed.
	XData::StringVectorMap _files;

	// The name of the chosen definition
	std::string _selection;

	// Pointer to the ReadableEditorDialog for updating the guiView.
	ReadableEditorDialog* _editorDialog;

	wxutil::Icon _xdataIcon;
	wxutil::Icon _folderIcon;

public:
	// Runs the dialog and returns the name of the chosen definition.
	static std::string run(const XData::StringVectorMap& files, ReadableEditorDialog* editorDialog);

	void visit(wxutil::TreeModel& store, wxutil::TreeModel::Row& row,
			   const std::string& path, bool isExplicit);

private:
	// private contructor called by the run method.
	XDataSelector(const XData::StringVectorMap& files, ReadableEditorDialog* editorDialog);

	void fillTree();

	void onSelectionChanged(wxDataViewEvent& ev); // view is manually bound
};

} //namespace ui
