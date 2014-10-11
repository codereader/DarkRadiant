#pragma once

#include <stdexcept>
#include "XDataLoader.h"
#include "wxutil/dialog/DialogBase.h"
#include "wxutil/TreeView.h"

namespace ui
{

class ReadableEditorDialog;

///////////////////////////// XdFileChooserDialog:
// Imports a given definition. If the definition has been found in multiple files,
// the dialog shows up asking which file to use.
class XdFileChooserDialog :
	public wxutil::DialogBase
{
public:
	struct ImportFailedException :
		public std::runtime_error
	{
		ImportFailedException(const std::string& what) : 
			std::runtime_error(what) 
		{}
	};

private:
	// Treestore enum
	struct ListStoreColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		ListStoreColumns() : 
			name(add(wxutil::TreeModel::Column::String)) 
		{}

		wxutil::TreeModel::Column name;

	};
	// A container for storing enumerated widgets
	ListStoreColumns _columns;
	wxutil::TreeModel* _listStore;
	wxutil::TreeView* _treeview;

	// The chosen filename.
	std::string _chosenFile;

	// Pointer to the ReadableEditorDialog for updating the guiView.
	ReadableEditorDialog* _editorDialog;

	// Definition name
	std::string _defName;
public:

	// Imports the definition given by defName and stores the result in newXData and the corresponding filename.
	// If the definition is found in mutliple files a dialog is shown asking the user which file to use.
	// @throws: ImportFailedException
	static int Import(const std::string& defName, XData::XDataPtr& newXData, 
		std::string& filename, XData::XDataLoaderPtr& loader, ReadableEditorDialog* editorDialog);

private:
	// Private constructor called by run.
	XdFileChooserDialog(const std::string& defName, const XData::XDataMap& xdMap, ReadableEditorDialog* editorDialog);

	void onOk();
	void onCancel();
	void onSelectionChanged(wxDataViewEvent& ev);
};

} // namespace ui
