// Project related
#include "ReadableEditorDialog.h"
#include "ReadableReloader.h"
#include "gui/GuiManager.h"

// General
#include "debugging/debugging.h"
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
	public std::enable_shared_from_this<GuiModule>
{
public:
	// RegisterableModule implementation
	const std::string& getName() const override
	{
		static std::string _name("GUI Editing");
		return _name;
	}

	const StringSet& getDependencies() const override
	{
		static StringSet _dependencies;

		if (_dependencies.empty())
		{
			_dependencies.insert(MODULE_COMMANDSYSTEM);
			_dependencies.insert(MODULE_FONTMANAGER);
			_dependencies.insert(MODULE_GAMEMANAGER);
			_dependencies.insert(MODULE_MAINFRAME);
			_dependencies.insert(MODULE_MAP);
			_dependencies.insert(MODULE_OPENGL);
			_dependencies.insert(MODULE_PREFERENCESYSTEM);
			_dependencies.insert(MODULE_RADIANT_APP);
			_dependencies.insert(MODULE_RENDERSYSTEM);
			_dependencies.insert(MODULE_SHADERSYSTEM);
			_dependencies.insert(MODULE_UIMANAGER);
			_dependencies.insert(MODULE_VIRTUALFILESYSTEM);
			_dependencies.insert(MODULE_XMLREGISTRY);
		}

		return _dependencies;
	}

	void initialiseModule(const IApplicationContext& ctx) override
	{
		rMessage() << getName() << "::initialiseModule called." << std::endl;

		GlobalCommandSystem().addCommand("ReadableEditorDialog", ui::ReadableEditorDialog::RunDialog);
		GlobalCommandSystem().addCommand("ReloadReadables", ui::ReadableReloader::run);

		GlobalMainFrame().signal_MainFrameConstructed().connect(
            sigc::mem_fun(this, &GuiModule::onMainFrameConstructed)
        );

		// Create the Readable Editor Preferences
		constructPreferences();
	}

private:
	void onMainFrameConstructed()
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
		IPreferencePage& page = GlobalPreferenceSystem().getPage(_("Settings/Readable Editor"));

		ComboBoxValueList options;

		options.push_back(_("Mod/xdata"));
		options.push_back(_("Mod Base/xdata"));
		options.push_back(_("Custom Folder"));

		page.appendCombo(_("XData Storage Folder"), ui::RKEY_READABLES_STORAGE_FOLDER, options);

		page.appendPathEntry(_("Custom Folder"), ui::RKEY_READABLES_CUSTOM_FOLDER, true);
	}
};
typedef std::shared_ptr<GuiModule> GuiModulePtr;

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	module::performDefaultInitialisation(registry);

	registry.registerModule(std::make_shared<GuiModule>());
	registry.registerModule(std::make_shared<gui::GuiManager>());
}
