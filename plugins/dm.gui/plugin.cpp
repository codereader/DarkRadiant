#include "imodule.h"

#include "ieventmanager.h"
#include "itextstream.h"
#include "debugging/debugging.h"
//#include "iimage.h"
#include "iuimanager.h"
#include "ifilesystem.h"

#include "ReadableEditorDialog.h"
#include "XData.h"

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

		//Testbench for xdata importer/exporter:
		readable::XDataPtrList testing;
		std::string filename;
		/*
		testing = readable::XData::importXDataFromFile("dm.gui_testing/training_mission.xd");
		testing[3]->xport("dm.gui_testing/training_mission_exported.xd", readable::Normal);
		testing[0]->xport("dm.gui_testing/training_mission_exported2.xd", readable::Normal);//*/
		//*
		filename = "dm.gui_testing/missing_all_one.xd";
		testing = readable::XData::importXDataFromFile(filename);
		if (testing[0])
		{
			if (testing[0]->xport("dm.gui_testing/mergetest.xd", readable::Merge) == readable::DefinitionExists)
				printf("Definition exists...\n");
			//if (testing[0]->xport("dm.gui_testing/mergetest.xd", readable::Merge) == readable::DefinitionExists)
			//	printf("Definition exists...\n");
			testing[0]->setContent(readable::Title, 1, readable::Right, "OVERWRITE WORKED...");
			testing[0]->xport("dm.gui_testing/mergetest.xd", readable::MergeOverwriteExisting);
		}//*/
		filename = "dm.gui_testing/missing_all_two.xd";
		testing = readable::XData::importXDataFromFile(filename);
		if (testing[0])
		{
			readable::FileStatus test = testing[0]->xport("dm.gui_testing/mergetest.xd", readable::Merge);//*/
			switch (test)
			{
			case readable::DefinitionExists: printf("Definition exists...\n"); break;
			case readable::DefinitionMismatch: printf("Definition Misnatch.\n"); break;
			case readable::MultipleDefinitions: printf("Multiple Definitions...\n"); break;
			case readable::AllOk: printf("All Ok...\n"); break;
			default: break;
			}
			test = testing[0]->xport("dm.gui_testing/mergetest.xd", readable::Overwrite);
			switch (test)
			{
			case readable::DefinitionExists: printf("Definition exists...\n"); break;
			case readable::DefinitionMismatch: printf("Definition Misnatch.\n"); break;
			case readable::MultipleDefinitions: printf("Multiple Definitions...\n"); break;
			case readable::AllOk: printf("All Ok...\n"); break;
			default: break;
			}
			test = testing[0]->xport("dm.gui_testing/mergetest.xd", readable::OverwriteMultDef);
			switch (test)
			{
			case readable::DefinitionExists: printf("Definition exists...\n"); break;
			case readable::DefinitionMismatch: printf("Definition Misnatch.\n"); break;
			case readable::MultipleDefinitions: printf("Multiple Definitions...\n"); break;
			case readable::AllOk: printf("All Ok...\n"); break;
			default: break;
			}
		}/*
		filename = "dm.gui_testing/num_guip_one.xd";
		testing = readable::XData::importXDataFromFile(filename);
		if (testing[0])
			testing[0]->xport("dm.gui_testing/mergetest.xd", readable::Merge);
		filename = "dm.gui_testing/num_guip_two.xd";
		testing = readable::XData::importXDataFromFile(filename);
		if (testing[0])
			testing[0]->xport("dm.gui_testing/mergetest.xd", readable::Merge);
		filename = "dm.gui_testing/quote_test.xd";
		testing = readable::XData::importXDataFromFile(filename);
		if (testing[0])
			testing[0]->xport("dm.gui_testing/mergetest.xd", readable::Merge);//*/

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
