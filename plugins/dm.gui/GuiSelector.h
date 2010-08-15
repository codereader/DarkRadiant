#ifndef _GUI_SELECTOR_H_
#define _GUI_SELECTOR_H_

#include "gtkutil/window/BlockingTransientWindow.h"

#include "gui/GuiManager.h"

#include "ReadableEditorDialog.h"

namespace ui
{

///////////////////////////// XDataSelector:
// Selector-Dialog for TwoSided and OneSided readable guis. Switching the pages of the notebook
// also toggles the according editing mode on the ReadableEditorDialog (TwoSided or OneSided).
// Selecting a gui definition updates the guiView for previewing.
class GuiSelector :
	public gtkutil::BlockingTransientWindow
{
private:
	// Reference to the calling ReadableEditorDialog
	ReadableEditorDialog& _editorDialog;

	// The name that was picked.
	std::string _name;

	// The notebook holding the tabs for one-sided and two-sided readables.
	GtkNotebook* _notebook;

	GtkTreeStore* _oneSidedStore;
	GtkTreeStore* _twoSidedStore;

	GtkWidget* _okButton;

	enum Result
	{
		RESULT_OK,
		RESULT_CANCELLED,
	};

	Result _result;

public:
	// Starts the GuiSelector and returns the name of the selected GUI or an empty string if the user canceled.
	// The dialog shows the twoSided treeview if twoSided is true.
	static std::string run(bool twoSided, ReadableEditorDialog& editorDialog);

protected:
	void _preShow();

private:
	GuiSelector(bool twoSided, ReadableEditorDialog& editorDialog);

	void fillTrees();

	GtkWidget* createInterface();
	GtkWidget* createButtons();
	GtkWidget* createOneSidedTreeView();
	GtkWidget* createTwoSidedTreeView();

	static void onCancel(GtkWidget* widget, GuiSelector* self);
	static void onOk(GtkWidget* widget, GuiSelector* self);
	static void onSelectionChanged(GtkTreeSelection *treeselection, GuiSelector* self);
	static void onPageSwitch(GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, GuiSelector* self);
};

} // namespace

#endif
