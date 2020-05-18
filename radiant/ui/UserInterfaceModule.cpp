#include "UserInterfaceModule.h"

#include "i18n.h"
#include "ilayer.h"
#include "ifilter.h"
#include "ientity.h"
#include "iorthocontextmenu.h"
#include "ieventmanager.h"
#include "imainframe.h"
#include "ishaders.h"

#include "wxutil/menu/CommandMenuItem.h"
#include "wxutil/MultiMonitor.h"

#include "module/StaticModule.h"

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
#include "ui/mousetool/ToolMappingDialog.h"
#include "ui/about/AboutDialog.h"
#include "ui/eclasstree/EClassTree.h"
#include "ui/entitylist/EntityList.h"
#include "textool/TexTool.h"
#include "modelexport/ExportAsModelDialog.h"
#include "ui/filters/FilterOrthoContextMenuItem.h"
#include "uimanager/colourscheme/ColourSchemeEditor.h"

namespace ui
{

namespace
{
	const char* const LAYER_ICON = "layers.png";
	const char* const CREATE_LAYER_TEXT = N_("Create Layer...");

	const char* const ADD_TO_LAYER_TEXT = N_("Add to Layer...");
	const char* const MOVE_TO_LAYER_TEXT = N_("Move to Layer...");
	const char* const REMOVE_FROM_LAYER_TEXT = N_("Remove from Layer...");

	const char* const SELECT_BY_FILTER_TEXT = N_("Select by Filter...");
	const char* const DESELECT_BY_FILTER_TEXT = N_("Deselect by Filter...");
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
		_dependencies.insert(MODULE_LAYERS);
		_dependencies.insert(MODULE_ORTHOCONTEXTMENU);
		_dependencies.insert(MODULE_UIMANAGER);
		_dependencies.insert(MODULE_FILTERSYSTEM);
		_dependencies.insert(MODULE_ENTITY);
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

	// Add the filter actions
	GlobalOrthoContextMenu().addItem(
		std::make_shared<FilterOrthoContextMenuItem>(_(SELECT_BY_FILTER_TEXT),
			FilterOrthoContextMenuItem::SelectByFilter),
		IOrthoContextMenu::SECTION_FILTER
	);

	GlobalOrthoContextMenu().addItem(
		std::make_shared<FilterOrthoContextMenuItem>(_(DESELECT_BY_FILTER_TEXT),
			FilterOrthoContextMenuItem::DeselectByFilter),
		IOrthoContextMenu::SECTION_FILTER
	);

	_eClassColourManager.reset(new EntityClassColourManager);
	_longOperationHandler.reset(new LongRunningOperationHandler);

	initialiseEntitySettings();
}

void UserInterfaceModule::shutdownModule()
{
	_coloursUpdatedConn.disconnect();
	_entitySettingsConn.disconnect();

	_longOperationHandler.reset();
	_eClassColourManager.reset();
}

void UserInterfaceModule::initialiseEntitySettings()
{
	auto& settings = GlobalEntityModule().getSettings();

	_entitySettingsConn = settings.signal_settingsChanged().connect(
		[]() { GlobalMainFrame().updateAllWindows(); }
	);

	applyEntityVertexColours();

	_coloursUpdatedConn = ColourSchemeEditor::signal_ColoursChanged().connect(
		sigc::mem_fun(this, &UserInterfaceModule::applyEntityVertexColours)
	);
}

void UserInterfaceModule::applyEntityVertexColours()
{
	auto& settings = GlobalEntityModule().getSettings();

	settings.setLightVertexColour(LightEditVertexType::StartEndDeselected, ColourSchemes().getColour("light_startend_deselected"));
	settings.setLightVertexColour(LightEditVertexType::StartEndSelected, ColourSchemes().getColour("light_startend_selected"));
	settings.setLightVertexColour(LightEditVertexType::Inactive, ColourSchemes().getColour("light_vertex_normal"));
	settings.setLightVertexColour(LightEditVertexType::Deselected, ColourSchemes().getColour("light_vertex_deselected"));
	settings.setLightVertexColour(LightEditVertexType::Selected, ColourSchemes().getColour("light_vertex_selected"));
}

void UserInterfaceModule::refreshShadersCmd(const cmd::ArgumentList& args)
{
	// Disable screen updates for the scope of this function
	auto blocker = GlobalMainFrame().getScopedScreenUpdateBlocker(_("Processing..."), _("Loading Shaders"));

	// Reload the Shadersystem, this will also trigger an 
	// OpenGLRenderSystem unrealise/realise sequence as the rendersystem
	// is attached to this class as Observer
	// We can't do this refresh() operation in a thread it seems due to context binding
	GlobalMaterialManager().refresh();

	GlobalMainFrame().updateAllWindows();
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
	GlobalCommandSystem().addCommand("MouseToolMappingDialog", ToolMappingDialog::ShowDialog);

	GlobalCommandSystem().addCommand("FindReplaceTextures", FindAndReplaceShader::ShowDialog);
	GlobalCommandSystem().addCommand("ShowCommandList", CommandList::ShowDialog);
	GlobalCommandSystem().addCommand("About", AboutDialog::showDialog);
	GlobalCommandSystem().addCommand("ShowUserGuide", Documentation::showUserGuide);
	GlobalCommandSystem().addCommand("ExportSelectedAsModelDialog", ExportAsModelDialog::ShowDialog);

	GlobalCommandSystem().addCommand("EntityClassTree", EClassTree::ShowDialog);
	GlobalCommandSystem().addCommand("EntityList", EntityList::toggle);

	GlobalCommandSystem().addCommand("RefreshShaders",
		std::bind(&UserInterfaceModule::refreshShadersCmd, this, std::placeholders::_1));
}

// Static module registration
module::StaticModule<UserInterfaceModule> userInterfaceModule;

}
