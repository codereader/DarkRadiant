#include "UserInterfaceModule.h"

#include "i18n.h"
#include "ilayer.h"
#include "iorthocontextmenu.h"
#include "ieventmanager.h"

#include "wxutil/menu/CommandMenuItem.h"
#include "wxutil/MultiMonitor.h"

#include "modulesystem/StaticModule.h"

#include "ui/prefdialog/GameSetupDialog.h"
#include "ui/layers/LayerOrthoContextMenuItem.h"
#include "ui/layers/LayerControlDialog.h"
#include "ui/overlay/OverlayDialog.h"
#include "ui/prefdialog/PrefDialog.h"
#include "ui/Documentation.h"
#include "log/Console.h"
#include "ui/lightinspector/LightInspector.h"
#include "ui/patch/PatchInspector.h"
#include "ui/surfaceinspector/SurfaceInspector.h"
#include "ui/transform/TransformDialog.h"
#include "ui/findshader/FindShader.h"
#include "map/FindMapElements.h"
#include "ui/mapinfo/MapInfoDialog.h"
#include "ui/commandlist/CommandList.h"
#include "ui/filterdialog/FilterDialog.h"
#include "ui/mousetool/ToolMappingDialog.h"
#include "ui/about/AboutDialog.h"
#include "ui/eclasstree/EClassTree.h"
#include "ui/entitylist/EntityList.h"
#include "textool/TexTool.h"
#include "modelexport/ExportAsModelDialog.h"

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

	wxutil::MultiMonitor::printMonitorInfo();

	registerUICommands();

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
		std::make_shared<LayerOrthoContextMenuItem>(_(ADD_TO_LAYER_TEXT), 
			LayerOrthoContextMenuItem::AddToLayer),
		IOrthoContextMenu::SECTION_LAYER
	);

	GlobalOrthoContextMenu().addItem(
		std::make_shared<LayerOrthoContextMenuItem>(_(MOVE_TO_LAYER_TEXT), 
			LayerOrthoContextMenuItem::MoveToLayer),
		IOrthoContextMenu::SECTION_LAYER
	);

	GlobalOrthoContextMenu().addItem(
		std::make_shared<LayerOrthoContextMenuItem>(_(REMOVE_FROM_LAYER_TEXT), 
			LayerOrthoContextMenuItem::RemoveFromLayer),
		IOrthoContextMenu::SECTION_LAYER
	);

	GlobalRadiant().signal_radiantStarted().connect(
		sigc::ptr_fun(LayerControlDialog::onRadiantStartup));
}

void UserInterfaceModule::shutdownModule()
{
}

void UserInterfaceModule::registerUICommands()
{
	TexTool::registerCommands();

	GlobalCommandSystem().addCommand("ProjectSettings", GameSetupDialog::Show);
	GlobalCommandSystem().addCommand("Preferences", PrefDialog::ShowPrefDialog);

	GlobalCommandSystem().addCommand("ToggleConsole", Console::toggle);
	GlobalCommandSystem().addCommand("ToggleLightInspector", LightInspector::toggleInspector);
	GlobalCommandSystem().addCommand("SurfaceInspector", SurfaceInspector::toggle);
	GlobalCommandSystem().addCommand("PatchInspector", PatchInspector::toggle);
	GlobalCommandSystem().addCommand("OverlayDialog", OverlayDialog::toggle);
	GlobalCommandSystem().addCommand("TransformDialog", TransformDialog::toggle);

	GlobalCommandSystem().addCommand("FindBrush", DoFind);

	GlobalCommandSystem().addCommand("MapInfo", MapInfoDialog::ShowDialog);
	GlobalCommandSystem().addCommand("EditFiltersDialog", FilterDialog::ShowDialog);
	GlobalCommandSystem().addCommand("MouseToolMappingDialog", ToolMappingDialog::ShowDialog);

	GlobalCommandSystem().addCommand("FindReplaceTextures", FindAndReplaceShader::ShowDialog);
	GlobalCommandSystem().addCommand("ShowCommandList", CommandList::ShowDialog);
	GlobalCommandSystem().addCommand("About", AboutDialog::showDialog);
	GlobalCommandSystem().addCommand("ShowUserGuide", Documentation::showUserGuide);
	GlobalCommandSystem().addCommand("ExportSelectedAsModelDialog", ExportAsModelDialog::ShowDialog);

	GlobalCommandSystem().addCommand("EntityClassTree", EClassTree::ShowDialog);
	GlobalCommandSystem().addCommand("EntityList", EntityList::toggle);

	// ----------------------- Bind Events ---------------------------------------

	GlobalEventManager().addCommand("ProjectSettings", "ProjectSettings");

	GlobalEventManager().addCommand("Preferences", "Preferences");

	GlobalEventManager().addCommand("ToggleConsole", "ToggleConsole");

	GlobalEventManager().addCommand("ToggleLightInspector", "ToggleLightInspector");
	GlobalEventManager().addCommand("SurfaceInspector", "SurfaceInspector");
	GlobalEventManager().addCommand("PatchInspector", "PatchInspector");
	GlobalEventManager().addCommand("OverlayDialog", "OverlayDialog");
	GlobalEventManager().addCommand("TransformDialog", "TransformDialog");

	GlobalEventManager().addCommand("FindBrush", "FindBrush");

	GlobalEventManager().addCommand("MapInfo", "MapInfo");
	GlobalEventManager().addCommand("EditFiltersDialog", "EditFiltersDialog");
	GlobalEventManager().addCommand("MouseToolMappingDialog", "MouseToolMappingDialog");

	GlobalEventManager().addCommand("FindReplaceTextures", "FindReplaceTextures");
	GlobalEventManager().addCommand("ShowCommandList", "ShowCommandList");
	GlobalEventManager().addCommand("About", "About");
	GlobalEventManager().addCommand("ShowUserGuide", "ShowUserGuide");
	GlobalEventManager().addCommand("ExportSelectedAsModelDialog", "ExportSelectedAsModelDialog");
	GlobalEventManager().addCommand("EntityClassTree", "EntityClassTree");
	GlobalEventManager().addCommand("EntityList", "EntityList");
}

// Static module registration
module::StaticModule<UserInterfaceModule> userInterfaceModule;

}
