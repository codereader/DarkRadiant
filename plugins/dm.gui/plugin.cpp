#include "imodule.h"

#include "ieventmanager.h"
#include "itextstream.h"
#include "debugging/debugging.h"
//#include "iimage.h"
#include "iuimanager.h"
#include "ifilesystem.h"

#include "ReadableEditorDialog.h"
#include "XData.h"
#include "XDataLoader.h"
#include "GuiManager.h"

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
			//_dependencies.insert(MODULE_IMAGELOADER + "TGA");
			//_dependencies.insert(MODULE_IMAGELOADER + "DDS");
		}

		return _dependencies;
	}
	
	virtual void initialiseModule(const ApplicationContext& ctx)
	{
		globalOutputStream() << getName() << "::initialiseModule called." << std::endl;

		GlobalCommandSystem().addCommand("ReadableEditorDialog", ui::ReadableEditorDialog::RunDialog);
		GlobalEventManager().addCommand("ReadableEditorDialog", "ReadableEditorDialog");

		GlobalUIManager().getMenuManager().add("main/view",
			"ReadableEditorDialog", ui::menuItem, 
			"Readable Editor", // caption
			"", // icon
			"ReadableEditorDialog"
		);

		gui::GuiManager::Instance().getGui("guis/mainmenu.gui");
		return;

		//Testbench for xdata importer/exporter:
		readable::XDataPtrList testing;
		std::string filename;
		readable::FileStatus test;
		if (boost::filesystem::exists("D:/games/Doom 3/darkmod/xdata/imptest_exported.xd"))
			boost::filesystem::remove("D:/games/Doom 3/darkmod/xdata/imptest_exported.xd");//*/
		try
		{
			//Newest tests:
			readable::XDataLoader* loader = new readable::XDataLoader();
			loader->retrieveXdInfo();
			//testing = loader->import("import_test.xd");
			testing.push_back(loader->importSingleDef("training_mission.xd", "trainer_stealth_flashbombs"));
			if (!testing.empty())
				if (testing[0])
					testing[0]->xport("D:/games/Doom 3/darkmod/xdata/single_imp.xd", readable::Normal);
			//testing[0]->xport("D:/games/Doom 3/darkmod/xdata/training_mission_exported.xd", readable::Merge);
			delete loader;
		}
		catch(std::runtime_error e) { printf(e.what()); }
		catch (...) {}
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
