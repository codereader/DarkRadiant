#include "XDataLoader.h"
#include "gtkutil/window/BlockingTransientWindow.h"
#include <gtk/gtk.h>
#include "ReadableEditorDialog.h"

namespace ui
{

///////////////////////////// XDataSelector:
// Runs a dialog for choosing XData definitions, which updates the guiView of the calling
// ReadableEditorDialog for previewing.
class XDataSelector :
	public gtkutil::BlockingTransientWindow
{
private:
	// The tree
	GtkTreeStore* _store;

	// A Map of XData files. Basically just the keyvalues are needed.
	XData::StringVectorMap _files;

	// The name of the chosen definition
	std::string _result;

	// Pointer to the ReadableEditorDialog for updating the guiView.
	ReadableEditorDialog* _editorDialog;

public:
	// Runs the dialog and returns the name of the chosen definition.
	static std::string run(const XData::StringVectorMap& files, ReadableEditorDialog* editorDialog);

private:
	//private contructor called by the run method.
	XDataSelector(const XData::StringVectorMap& files, ReadableEditorDialog* editorDialog);

	void fillTree();

	// Helper functions to create GUI components
	GtkWidget* createTreeView();
	GtkWidget* createButtons();

	static void onCancel(GtkWidget* widget, XDataSelector* self);
	static void onOk(GtkWidget* widget, XDataSelector* self);
	static void onSelectionChanged(GtkTreeSelection *treeselection, XDataSelector* self);
};

} //namespace ui
