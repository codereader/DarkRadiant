#include <sigc++/connection.h>
#include <wx/frame.h>

#include "iscript.h"
#include "ui/imenumanager.h"
#include "ui/imainframe.h"
#include "i18n.h"
#include "ui/igroupdialog.h"
#include "module/StaticModule.h"

#include "ScriptMenu.h"
#include "ScriptWindow.h"

namespace ui
{

class ScriptUserInterfaceModule :
	public RegisterableModule
{
private:
	ScriptMenuPtr _scriptMenu;

	sigc::connection _scriptsReloadedConn;

public:
	// RegisterableModule
	const std::string& getName() const override
	{
		static std::string _name("ScriptUserInterface");
		return _name;
	}

	const StringSet& getDependencies() const override
	{
        static StringSet _dependencies
        {
            MODULE_SCRIPTING_SYSTEM,
            MODULE_MENUMANAGER,
            MODULE_MAINFRAME,
        };

		return _dependencies;
	}

	void initialiseModule(const IApplicationContext& ctx) override
	{
		// Bind the reloadscripts command to the menu
        GlobalMenuManager().insert("main/file/refreshShaders", 	// menu location path
			"ReloadScripts", // name
			menu::ItemType::Item,	// type
			_("Reload Scripts"),	// caption
			"",	// icon
			"ReloadScripts"); // event name

		// Subscribe to get notified as soon as Radiant is fully initialised
		GlobalMainFrame().signal_MainFrameConstructed().connect(
			sigc::mem_fun(this, &ScriptUserInterfaceModule::onMainFrameConstructed)
		);

		_scriptsReloadedConn = GlobalScriptingSystem().signal_onScriptsReloaded()
			.connect(sigc::mem_fun(this, &ScriptUserInterfaceModule::onScriptsReloaded));
	}

	void shutdownModule() override
	{
		_scriptsReloadedConn.disconnect();
		_scriptMenu.reset();
	}

private:
	void onScriptsReloaded()
	{
		_scriptMenu.reset();
		_scriptMenu = std::make_shared<ScriptMenu>();
	}

	void onMainFrameConstructed()
	{
		_scriptMenu = std::make_shared<ScriptMenu>();

		// Add the scripting widget to the groupdialog
		IGroupDialog::PagePtr page(new IGroupDialog::Page);

		page->name = "ScriptWindow";
		page->windowLabel = _("Script");
		page->page = new ScriptWindow(GlobalMainFrame().getWxTopLevelWindow());
		page->tabIcon = "icon_script.png";
		page->tabLabel = _("Script");
		page->position = IGroupDialog::Page::Position::Console - 10; // insert before console

		GlobalGroupDialog().addPage(page);
	}
};

module::StaticModuleRegistration<ScriptUserInterfaceModule> scriptUserInterfaceModule;

}
