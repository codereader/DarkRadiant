#include "imodule.h"

#include "ieventmanager.h"
//#include "iuimanager.h"
//#include "ientityinspector.h"
//#include "icommandsystem.h"
#include "itextstream.h"
//#include "imainframe.h"
#include "debugging/debugging.h"
//#include "iimage.h"
#include "ifilesystem.h"

#include "ReadableEditorDialog.h"
//#include "FontLoader.h"
#include "XDataManager.h"

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

		//Temporary Testbench for exporter:
		readable::OneSidedXDataPtr test(new readable::OneSidedXData("test_onesided"));
		test->_guiPage.push_back("guitest_page1");
		test->_guiPage.push_back("guitest_page2");
		test->_numPages = 2;
		test->_sndPageTurn = "test_sndPageTurn";
		test->_pageTitle.push_back("");
		test->_pageTitle.push_back("title2\ntitle2 second line");
		test->_pageBody.push_back("blah blah blah \n blah and stuff");
		test->_pageBody.push_back("");
		readable::XDataManager::exportXData("dm.gui_testing/test_onesided.xd", *test, readable::Normal);

		readable::TwoSidedXDataPtr test2(new readable::TwoSidedXData("test_twosided"));
		test2->_guiPage.push_back("twoside_gui_p1");
		test2->_guiPage.push_back("twoside_gui_p2");
		test2->_numPages = 2;
		test2->_sndPageTurn = "test_sndPageTurn_twoside";
		test2->_pageLeftTitle.push_back("");
		test2->_pageLeftTitle.push_back("l_title2\nlololo");
		test2->_pageRightTitle.push_back("r_title1\nblah");
		test2->_pageRightTitle.push_back("");
		test2->_pageLeftBody.push_back("left1: this is a text\nwith new lines\n\t and tabs");
		test2->_pageLeftBody.push_back("");
		test2->_pageRightBody.push_back("");
		test2->_pageRightBody.push_back("right2: this is a text\nwith new lines\n\t and tabs");
		readable::XDataManager::exportXData("dm.gui_testing/test_twosided.xd", *test2, readable::Normal);
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
