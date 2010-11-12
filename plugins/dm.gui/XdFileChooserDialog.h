#ifndef _XDATA_FILE_CHOOSER_DIALOG_H_
#define _XDATA_FILE_CHOOSER_DIALOG_H_

#include "XDataLoader.h"
#include "gtkutil/window/BlockingTransientWindow.h"
#include <gtkmm/liststore.h>

namespace Gtk
{
	class TreeView;
}

namespace ui
{

class ReadableEditorDialog;

///////////////////////////// XdFileChooserDialog:
// Imports a given definition. If the definition has been found in multiple files,
// the dialog shows up asking which file to use.
class XdFileChooserDialog :
	public gtkutil::BlockingTransientWindow
{
public:
	enum Result
	{
		RESULT_OK,
		RESULT_CANCEL,
		RESULT_IMPORT_FAILED,
		NUM_RESULTS,
	};

private:
	// Treestore enum
	struct ListStoreColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		ListStoreColumns() { add(name); }

		Gtk::TreeModelColumn<Glib::ustring> name;

	};
	// A container for storing enumerated widgets
	ListStoreColumns _columns;
	Glib::RefPtr<Gtk::ListStore> _listStore;
	Gtk::TreeView* _treeview;

	// Return value
	Result _result;

	// The chosen filename.
	std::string _chosenFile;

	// Pointer to the ReadableEditorDialog for updating the guiView.
	ReadableEditorDialog& _editorDialog;

	// Definition name
	std::string _defName;
public:

	// Imports the definition given by defName and stores the result in newXData and the corresponding filename.
	// If the definition is found in mutliple files an dialog shows up, asking the user which file to use.
	static Result import(const std::string& defName, XData::XDataPtr& newXData, std::string& filename, XData::XDataLoaderPtr& loader, ReadableEditorDialog& editorDialog);

private:
	// Private constructor called by run.
	XdFileChooserDialog(const std::string& defName, const XData::XDataMap& xdMap, ReadableEditorDialog& editorDialog);

	void onOk();
	void onCancel();
	void onSelectionChanged();
};

} // namespace ui

#endif
