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
#include "ifilesystem.h"
#include "ifonts.h"
#include "igame.h"
#include "igl.h"
#include "ui/imainframe.h"
#include "imap.h"
#include "ipreferencesystem.h"
#include "iradiant.h"
#include "iregistry.h"
#include "irender.h"
#include "ishaders.h"
#include "ui/imenumanager.h"
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
        static StringSet _dependencies
        {
            MODULE_COMMANDSYSTEM,
            MODULE_FONTMANAGER,
            MODULE_GAMEMANAGER,
            MODULE_MAINFRAME,
            MODULE_MAP,
            MODULE_OPENGL,
            MODULE_PREFERENCESYSTEM,
            MODULE_RENDERSYSTEM,
            MODULE_SHADERSYSTEM,
            MODULE_MENUMANAGER,
            MODULE_VIRTUALFILESYSTEM,
            MODULE_XMLREGISTRY,
        };

		return _dependencies;
	}

	void initialiseModule(const IApplicationContext& ctx) override
	{
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
		ui::menu::IMenuManager& mm = GlobalMenuManager();

		mm.add("main/entity",
			"ReadableEditorDialog", ui::menu::ItemType::Item,
			_("Readable Editor"), // caption
			"book.png", // icon
			"ReadableEditorDialog"
		);

		mm.insert("main/file/reloadDecls",
			"ReloadReadables", ui::menu::ItemType::Item,
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
