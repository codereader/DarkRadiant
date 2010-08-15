#ifndef _XDATA_SELECTOR_H_
#define _XDATA_SELECTOR_H_

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

	GtkWidget* _okButton;

	// A Map of XData files. Basically just the keyvalues are needed.
	XData::StringVectorMap _files;

	// The name of the chosen definition
	std::string _selection;

	// Pointer to the ReadableEditorDialog for updating the guiView.
	ReadableEditorDialog* _editorDialog;

	enum Result
	{
		RESULT_OK,
		RESULT_CANCELLED,
	};

	Result _result;

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

#endif
