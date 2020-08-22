#include "FindBrush.h"

#include "i18n.h"
#include "idialogmanager.h"
#include "iselection.h"
#include "scene/SelectionIndex.h"

namespace ui
{

void FindBrushDialog::Show(const cmd::ArgumentList& args)
{
	ui::IDialogPtr dialog = GlobalDialogManager().createDialog(_("Find Brush"));

	ui::IDialog::Handle entityEntry = dialog->addEntryBox(_("Entity Number:"));
	ui::IDialog::Handle brushEntry = dialog->addEntryBox(_("Brush Number:"));

	// Pre-fill the entry boxes with the index of the current selection, if applicable
	if (GlobalSelectionSystem().countSelected() == 1)
	{
		auto node = GlobalSelectionSystem().ultimateSelected();

		auto indexPair = scene::getNodeIndices(node);

		dialog->setElementValue(entityEntry, string::to_string(indexPair.first));
		dialog->setElementValue(brushEntry, string::to_string(indexPair.second));
	}
	else
	{
		dialog->setElementValue(entityEntry, 0);
		dialog->setElementValue(brushEntry, 0);
	}

	if (dialog->run() == ui::IDialog::RESULT_OK)
	{
		std::string entityValue = dialog->getElementValue(entityEntry);
		std::string brushValue = dialog->getElementValue(brushEntry);

		GlobalCommandSystem().executeCommand(scene::SELECT_NODE_BY_INDEX_CMD,
			string::convert<int>(entityValue), string::convert<int>(brushValue));
	}
}

}
