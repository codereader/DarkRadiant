#include "GuiSelector.h"
#include "gtkutil/VFSTreePopulator.h"
#include "GuiInserter.h"
#include "imainframe.h"

namespace ui
{
	namespace
	{
		const std::string WINDOW_TITLE("Choose a Gui Definition...");
	}


	GuiSelector::GuiSelector(bool twoSided) :
		gtkutil::BlockingTransientWindow(WINDOW_TITLE, GlobalMainFrame().getTopLevelWindow()),
		_name("")
	{
		fillTrees();
	}

	void GuiSelector::fillTrees()
	{
		gtkutil::VFSTreePopulator popOne(_storeOneSided);
		gtkutil::VFSTreePopulator popTwo(_storeTwoSided);

		gui::GuiManager::Instance().refreshGuiDefinitions();
		gui::GuiManager::GuiMap m = gui::GuiManager::Instance().getGuiDefinitions();
		for (gui::GuiManager::GuiMap::iterator it = m.begin(); it != m.end(); it++)
		{
			//identify readables and differ between onesided and twosided ones. Add the gui to the according tree.
		}

		GuiInserter inserter;
		popOne.forEachNode(inserter);
		popTwo.forEachNode(inserter);
	}

	std::string GuiSelector::run(bool twoSided)
	{
		GuiSelector dialog(twoSided);
		dialog.show();

		return dialog._name;
	}

	GtkWidget* GuiSelector::createInterface()
	{
		return gtk_hbox_new(FALSE,0);
	}

	GtkWidget* GuiSelector::createOneSidedTreeView()
	{
		return gtk_hbox_new(FALSE,0);
	}

	GtkWidget* GuiSelector::createTwoSidedTreeView()
	{
		return gtk_hbox_new(FALSE,0);
	}

}