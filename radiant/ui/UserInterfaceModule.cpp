#include "UserInterfaceModule.h"

#include "i18n.h"
#include "ilayer.h"
#include "iorthocontextmenu.h"

#include "wxutil/menu/CommandMenuItem.h"

#include "modulesystem/StaticModule.h"

#include "ui/layers/LayerOrthoContextMenuItem.h"
#include "ui/layers/LayerControlDialog.h"

namespace ui
{

namespace
{
	const char* const LAYER_ICON = "layers.png";
	const char* const CREATE_LAYER_TEXT = N_("Create Layer...");

	const char* const ADD_TO_LAYER_TEXT = N_("Add to Layer...");
	const char* const MOVE_TO_LAYER_TEXT = N_("Move to Layer...");
	const char* const REMOVE_FROM_LAYER_TEXT = N_("Remove from Layer...");
}

const std::string& UserInterfaceModule::getName() const
{
	static std::string _name("UserInterfaceModule");
	return _name;
}

const StringSet& UserInterfaceModule::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_LAYERSYSTEM);
		_dependencies.insert(MODULE_ORTHOCONTEXTMENU);
		_dependencies.insert(MODULE_UIMANAGER);
	}

	return _dependencies;
}

void UserInterfaceModule::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	// Register LayerControlDialog
	GlobalCommandSystem().addCommand("ToggleLayerControlDialog", LayerControlDialog::toggle);
	GlobalEventManager().addCommand("ToggleLayerControlDialog", "ToggleLayerControlDialog");

	// Create a new menu item connected to the CreateNewLayer command
	GlobalOrthoContextMenu().addItem(std::make_shared<wxutil::CommandMenuItem>(
			new wxutil::IconTextMenuItem(_(CREATE_LAYER_TEXT), LAYER_ICON), "CreateNewLayer"),
		IOrthoContextMenu::SECTION_LAYER
	);

	// Add the orthocontext menu's layer actions
	GlobalOrthoContextMenu().addItem(
		std::make_shared<LayerOrthoContextMenuItem>(_(ADD_TO_LAYER_TEXT), LayerOrthoContextMenuItem::AddToLayer),
		IOrthoContextMenu::SECTION_LAYER
	);

	GlobalOrthoContextMenu().addItem(
		std::make_shared<LayerOrthoContextMenuItem>(_(MOVE_TO_LAYER_TEXT), LayerOrthoContextMenuItem::MoveToLayer),
		IOrthoContextMenu::SECTION_LAYER
	);

	GlobalOrthoContextMenu().addItem(
		std::make_shared<LayerOrthoContextMenuItem>(_(REMOVE_FROM_LAYER_TEXT), LayerOrthoContextMenuItem::RemoveFromLayer),
		IOrthoContextMenu::SECTION_LAYER
	);
}

void UserInterfaceModule::shutdownModule()
{
}

// Static module registration
module::StaticModule<UserInterfaceModule> userInterfaceModule;

}
