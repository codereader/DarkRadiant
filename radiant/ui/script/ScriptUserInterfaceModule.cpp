#include <sigc++/connection.h>

#include "iscript.h"
#include "icommandsystem.h"
#include "ui/imenumanager.h"
#include "ui/imainframe.h"
#include "i18n.h"
#include "ui/iuserinterface.h"
#include "module/StaticModule.h"

#include "ScriptMenu.h"
#include "ScriptPanel.h"

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
            MODULE_USERINTERFACE,
            MODULE_COMMANDSYSTEM,
        };

		return _dependencies;
	}

	void initialiseModule(const IApplicationContext& ctx) override
	{
		// Bind the reloadscripts command to the menu
        GlobalMenuManager().insert("main/file/reloadDecls", 	// menu location path
			"ReloadScripts", // name
			menu::ItemType::Item,	// type
			_("Reload Scripts"),	// caption
			"icon_script.png",	// icon
			"ReloadScripts"); // event name

		// Subscribe to get notified as soon as Radiant is fully initialised
		GlobalMainFrame().signal_MainFrameConstructed().connect(
			sigc::mem_fun(this, &ScriptUserInterfaceModule::onMainFrameConstructed)
		);

		_scriptsReloadedConn = GlobalScriptingSystem().signal_onScriptsReloaded()
			.connect(sigc::mem_fun(this, &ScriptUserInterfaceModule::onScriptsReloaded));

        GlobalUserInterface().registerControl(std::make_shared<ScriptPanel>());
	}

	void shutdownModule() override
	{
        GlobalUserInterface().unregisterControl(ScriptPanel::Name);

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

        GlobalMainFrame().addControl(ScriptPanel::Name, IMainFrame::ControlSettings
        {
            IMainFrame::Location::PropertyPanel,
            true
        });
	}
};

module::StaticModuleRegistration<ScriptUserInterfaceModule> scriptUserInterfaceModule;

}
