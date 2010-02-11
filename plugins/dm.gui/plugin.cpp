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

//#include "AIHeadPropertyEditor.h"
#include "ReadableEditorDialog.h"
#include "FontLoader.h"

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
			_dependencies.insert(MODULE_VIRTUALFILESYSTEM);		//????????????????????? necessary?
			//_dependencies.insert(MODULE_ENTITYINSPECTOR);
			_dependencies.insert(MODULE_EVENTMANAGER);
			//_dependencies.insert(MODULE_UIMANAGER);
			_dependencies.insert(MODULE_COMMANDSYSTEM);
			_dependencies.insert(MODULE_IMAGELOADER + "TGA");
			_dependencies.insert(MODULE_IMAGELOADER + "DDS");
			//_dependencies.insert(MODULE_MAINFRAME);
		}

		return _dependencies;
	}
	
	virtual void initialiseModule(const ApplicationContext& ctx)
	{
		globalOutputStream() << getName() << "::initialiseModule called." << std::endl;
		
		/* ???
		// Associated "def_head" with an empty property editor instance
		GlobalEntityInspector().registerPropertyEditor(
			ui::DEF_HEAD_KEY, ui::IPropertyEditorPtr(new ui::AIHeadPropertyEditor())
		);//*/

		GlobalCommandSystem().addCommand("ReadableEditorDialog", ui::ReadableEditorDialog::RunDialog);
		GlobalEventManager().addCommand("ReadableEditorDialog", "ReadableEditorDialog");

		GlobalUIManager().getMenuManager().add("main/view",
			"ReadableEditorDialog", ui::menuItem, 
			"Readable Editor", // caption
			"", // icon
			"ReadableEditorDialog"
		);

		try
		{
			readable::FontReaderPtr test(new readable::FontLoader("D:\\Data\\baumannm\\darkmod\\d3\\darkmod\\fonts\\english\\andrew_script\\fontImage_48.dat"));
		}
		catch (...)
		{

		}
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
