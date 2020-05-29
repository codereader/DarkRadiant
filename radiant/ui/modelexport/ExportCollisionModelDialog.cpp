#include "ExportCollisionModelDialog.h"

#include "i18n.h"
#include "selectionlib.h"
#include "command/ExecutionNotPossible.h"
#include "ui/modelselector/ModelSelector.h"

namespace ui
{

void ExportCollisionModelDialog::Show(const cmd::ArgumentList& args)
{
	// Check the current selection state
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	if (info.totalCount != info.entityCount || info.entityCount != 1)
	{
		throw cmd::ExecutionNotPossible(_("Can't export, create and select a func_* entity\
				containing the collision hull primitives."));
	}

	auto modelAndSkin = ModelSelector::chooseModel("", false, false);

	if (modelAndSkin.model.empty())
	{
		return;
	}

	GlobalCommandSystem().executeCommand("ExportSelectedAsCollisionModel", modelAndSkin.model);
}

}
