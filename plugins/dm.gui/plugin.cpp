// Project related
#include "ReadableEditorDialog.h"
#include "ReadableReloader.h"
#include "gui/GuiManager.h"

// General
#include "debugging/debugging.h"
#include <boost/enable_shared_from_this.hpp>
#include "i18n.h"

// Modules
#include "icommandsystem.h"
#include "ientity.h"
#include "ieventmanager.h"
#include "ifilesystem.h"
#include "ifonts.h"
#include "igame.h"
#include "igl.h"
#include "imainframe.h"
#include "imap.h"
#include "ipreferencesystem.h"
#include "iradiant.h"
#include "iregistry.h"
#include "irender.h"
#include "ishaders.h"
#include "iuimanager.h"
#include "iarchive.h"


class GuiModule :
	public RegisterableModule,
	public boost::enable_shared_from_this<GuiModule>
{
public:
	// RegisterableModule implementation
	virtual const std::string& getName() const
	{
		static std::string _name("GUI Editing");
		return _name;
	}

	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies;

		if (_dependencies.empty())
		{
			_dependencies.insert(MODULE_COMMANDSYSTEM);
			_dependencies.insert(MODULE_EVENTMANAGER);
			_dependencies.insert(MODULE_FONTMANAGER);
			_dependencies.insert(MODULE_GAMEMANAGER);
			_dependencies.insert(MODULE_MAINFRAME);
			_dependencies.insert(MODULE_MAP);
			_dependencies.insert(MODULE_OPENGL);
			_dependencies.insert(MODULE_PREFERENCESYSTEM);
			_dependencies.insert(MODULE_RADIANT);
			_dependencies.insert(MODULE_RENDERSYSTEM);
			_dependencies.insert(MODULE_SHADERSYSTEM);
			_dependencies.insert(MODULE_UIMANAGER);
			_dependencies.insert(MODULE_VIRTUALFILESYSTEM);
			_dependencies.insert(MODULE_XMLREGISTRY);
		}

		return _dependencies;
	}

	virtual void initialiseModule(const ApplicationContext& ctx)
	{
		rMessage() << getName() << "::initialiseModule called." << std::endl;

		GlobalCommandSystem().addCommand("ReadableEditorDialog", ui::ReadableEditorDialog::RunDialog);
		GlobalEventManager().addCommand("ReadableEditorDialog", "ReadableEditorDialog");

		GlobalCommandSystem().addCommand("ReloadReadables", ui::ReadableReloader::run);
		GlobalEventManager().addCommand("ReloadReadables", "ReloadReadables");

		GlobalRadiant().signal_radiantStarted().connect(
            sigc::mem_fun(this, &GuiModule::onRadiantStartup)
        );

		// Search the VFS for GUIs
		gui::GuiManager::Instance().findGuis();

		// Create the Readable Editor Preferences
		constructPreferences();
	}

	void onRadiantStartup()
	{
		// Add menu items on radiant startup, to ensure that all menu items are existent at this point
		IMenuManager& mm = GlobalUIManager().getMenuManager();

		mm.add("main/entity",
			"ReadableEditorDialog", ui::menuItem,
			_("Readable Editor"), // caption
			"book.png", // icon
			"ReadableEditorDialog"
		);

		mm.insert("main/file/refreshShaders",
			"ReloadReadables", ui::menuItem,
			_("Reload Readable Guis"), // caption
			"book.png", // icon
			"ReloadReadables"
		);
	}

	// Adds the preference settings to the prefdialog
	void constructPreferences()
	{
		// Add a page to the given group
		PreferencesPagePtr page = GlobalPreferenceSystem().getPage(_("Settings/Readable Editor"));

		ComboBoxValueList options;

		options.push_back(_("Mod/xdata"));
		options.push_back(_("Mod Base/xdata"));
		options.push_back(_("Custom Folder"));

		page->appendCombo(_("XData Storage Folder"), ui::RKEY_READABLES_STORAGE_FOLDER, options);

		page->appendPathEntry(_("Custom Folder"), ui::RKEY_READABLES_CUSTOM_FOLDER, true);
	}

	void shutdownModule()
	{
		gui::GuiManager::Instance().clear();
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
