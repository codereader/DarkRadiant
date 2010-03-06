#include "imodule.h"

#include "ieventmanager.h"
#include "itextstream.h"
#include "debugging/debugging.h"
#include "iuimanager.h"
#include "ifilesystem.h"
#include "irender.h"
#include "igl.h"
#include "imap.h"
#include "igame.h"
#include "ipreferencesystem.h"

#include "ReadableEditorDialog.h"
#include "ReadableReloader.h"
#include "gui/GuiManager.h"

class GuiModule : 
	public RegisterableModule
{
public:
	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name("Gui Editing");
		return _name;
	}
	
	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies;

		if (_dependencies.empty())
		{
			_dependencies.insert(MODULE_EVENTMANAGER);
			_dependencies.insert(MODULE_COMMANDSYSTEM);
			_dependencies.insert(MODULE_VIRTUALFILESYSTEM);
			_dependencies.insert(MODULE_RENDERSYSTEM);
			_dependencies.insert(MODULE_OPENGL);
			_dependencies.insert(MODULE_MAP);
			_dependencies.insert(MODULE_GAMEMANAGER);
			_dependencies.insert(MODULE_PREFERENCESYSTEM);
		}

		return _dependencies;
	}
	
	virtual void initialiseModule(const ApplicationContext& ctx)
	{
		globalOutputStream() << getName() << "::initialiseModule called." << std::endl;

		GlobalCommandSystem().addCommand("ReadableEditorDialog", ui::ReadableEditorDialog::RunDialog);
		GlobalEventManager().addCommand("ReadableEditorDialog", "ReadableEditorDialog");

		IMenuManager& mm = GlobalUIManager().getMenuManager();

		mm.add("main/entity",
			"ReadableEditorDialog", ui::menuItem, 
			"Readable Editor", // caption
			"book.png", // icon
			"ReadableEditorDialog"
		);

		GlobalCommandSystem().addCommand("ReloadReadables", ui::ReadableReloader::run);
		GlobalEventManager().addCommand("ReloadReadables", "ReloadReadables");

		mm.insert("main/file/refreshShaders",
			"ReloadReadables", ui::menuItem, 
			"Reload Readables", // caption
			"book.png", // icon
			"ReloadReadables"
		);

		// Search the VFS for GUIs
		gui::GuiManager::Instance().findGuis();

		// Create the Readable Editor Preferences
		//constructPreferences();
	}

	// Adds the preference settings to the prefdialog
	void constructPreferences()
	{
		// Add a page to the given group
		PreferencesPagePtr page = GlobalPreferenceSystem().getPage("Settings/Primitives");

		std::list<std::string> options;

		options.push_back("Mod");
		options.push_back("Mod Base");
		options.push_back("Custom Folder");

		page->appendCombo("XData Storage Folder", "user/ui/gui/storageFolder", options);

		page->appendPathEntry("Custom Folder", "user/ui/gui/customFolder", true);
	}
};
typedef boost::shared_ptr<GuiModule> GuiModulePtr;

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	registry.registerModule(GuiModulePtr(new GuiModule));
	
	// Initialize the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());
	
	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);

	// Set up the assertion handler
	GlobalErrorHandler() = registry.getApplicationContext().getErrorHandlingFunction();
}
