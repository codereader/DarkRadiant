#include "XDataLoader.h"
#include "gtkutil/window/BlockingTransientWindow.h"
#include "ReadableEditorDialog.h"

namespace ui
{

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
	// A container for storing enumerated widgets
	GtkWidget* _treeview;

	// Return value
	Result _result;

	// The chosen filename.
	std::string _chosenFile;

	// Pointer to the ReadableEditorDialog for updating the guiView.
	ReadableEditorDialog* _editorDialog;

	// Definition name
	std::string _defName;
public:

	// Imports the definition given by defName and stores the result in newXData and the corresponding filename.
	// If the definition is found in mutliple files an dialog shows up, asking the user which file to use.
	static Result import(const std::string& defName, XData::XDataPtr& newXData, std::string& filename, XData::XDataLoaderPtr& loader, ReadableEditorDialog* editorDialog);

private:
	// Private constructor called by run.
	XdFileChooserDialog(const std::string& defName, const XData::XDataMap& xdMap, ReadableEditorDialog* editorDialog);

	static void onOk(GtkWidget* widget, XdFileChooserDialog* self);
	static void onCancel(GtkWidget* widget, XdFileChooserDialog* self);
	static void onSelectionChanged(GtkTreeSelection *treeselection, XdFileChooserDialog* self);
};

} // namespace ui
