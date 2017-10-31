#include "GameSetupDialog.h"

#include "modulesystem/ModuleRegistry.h"

namespace ui
{

GameSetupDialog::GameSetupDialog(wxWindow* parent) :
	DialogBase(_("Game Setup"), parent)
{}

void GameSetupDialog::Show(const cmd::ArgumentList& args)
{
	// greebo: Check if the mainframe module is already "existing". It might be
	// uninitialised if this dialog is shown during DarkRadiant startup
	wxWindow* parent = module::GlobalModuleRegistry().moduleExists(MODULE_MAINFRAME) ?
		GlobalMainFrame().getWxTopLevelWindow() : nullptr;

	GameSetupDialog* dialog = new GameSetupDialog(parent);

	dialog->ShowModal();
	
	dialog->Destroy();
}

}
