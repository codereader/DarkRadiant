#include "gtkutil/window/BlockingTransientWindow.h"
#include "gui/GuiManager.h"
#include <gtk/gtk.h>

namespace ui
{

	class GuiSelector :
		public gtkutil::BlockingTransientWindow
	{
	private:
		// The trees
		GtkTreeStore *_storeOneSided, *_storeTwoSided;

		// The name that was picked.
		std::string _name;
	public:
		// Starts the GuiSelector and returns the name of the selected GUI or an empty string if the user canceled.
		// The dialog shows the twoSided treeview if twoSided is true.
		static std::string run(bool twoSided);

	private:
		GuiSelector(bool twoSided);

		void fillTrees();

		GtkWidget* createInterface();
		GtkWidget* createOneSidedTreeView();
		GtkWidget* createTwoSidedTreeView();
	};


}