#include "gtkutil/window/BlockingTransientWindow.h"
#include "gui/GuiManager.h"
#include <gtk/gtk.h>
#include "ReadableEditorDialog.h"

namespace ui
{

	class GuiSelector :
		public gtkutil::BlockingTransientWindow
	{
	private:
		// ...and their treeViews
		GtkTreeView *_treeViewOne, *_treeViewTwo;

		// The name that was picked.
		std::string _name;

		// The notebook holding the tabs for one-sided and two-sided readables.
		GtkNotebook* _notebook;

		// Static treestores. These are likely not to change during runtime and creating the treestores takes some time...
		GtkTreeStore* getOneSidedStore();
		GtkTreeStore* getTwoSidedStore();

		// The selections of the trees.
		GtkTreeSelection *_selectOne, *_selectTwo;

		// Pointer to the calling ReadableEditorDialog
		ReadableEditorDialog* _editorDialog;
	public:
		// Starts the GuiSelector and returns the name of the selected GUI or an empty string if the user canceled.
		// The dialog shows the twoSided treeview if twoSided is true.
		static std::string run(bool twoSided, ReadableEditorDialog* editorDialog);

	private:
		GuiSelector(bool twoSided, ReadableEditorDialog* editorDialog);

		bool fillTrees();

		static gint treeViewSortFunc(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data);

		GtkWidget* createInterface();
		GtkWidget* createButtons();
		GtkWidget* createOneSidedTreeView();
		GtkWidget* createTwoSidedTreeView();

		static void onCancel(GtkWidget* widget, GuiSelector* self);
		static void onOk(GtkWidget* widget, GuiSelector* self);
		static void onSelectionChanged(GtkTreeSelection *treeselection, GuiSelector* self);
		static void onPageSwitch(GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, GuiSelector* self);
	};


}