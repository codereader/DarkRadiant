#include "XDataLoader.h"
#include "gtkutil/window/BlockingTransientWindow.h"
#include <gtk/gtk.h>

namespace ui
{
	class XDataSelector :
		public gtkutil::BlockingTransientWindow
	{
	private:
		// The list of files
		const XData::StringVectorMap _files;

		// The tree
		GtkTreeStore* _store;

		// The name of the chosen definition
		std::string _result;

		// The treeView
		GtkTreeView* _treeView;

	public:
		// Runs the dialog and returns the name of the chosen definition.
		static std::string run(const XData::StringVectorMap& files);

	private:
		//private contructor called by the run method.
		XDataSelector(const XData::StringVectorMap& files);

		void fillTree();

		static gint treeViewSortFunc(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data);

		// Helper functions to create GUI components
		GtkWidget* createTreeView();
		GtkWidget* createButtons();

		static void onCancel(GtkWidget* widget, XDataSelector* self);
		static void onOk(GtkWidget* widget, XDataSelector* self);
	};
}