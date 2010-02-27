#include "imodule.h"

#include "ieventmanager.h"
#include "itextstream.h"
#include "debugging/debugging.h"
#include "iregistry.h"
#include "iuimanager.h"
#include "ifilesystem.h"
#include "irender.h"
#include "igl.h"
#include "imap.h"

#include "ReadableEditorDialog.h"
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
			_dependencies.insert(MODULE_XMLREGISTRY);
			_dependencies.insert(MODULE_RENDERSYSTEM);
			_dependencies.insert(MODULE_OPENGL);
			_dependencies.insert(MODULE_MAP);
		}

		return _dependencies;
	}
	
	virtual void initialiseModule(const ApplicationContext& ctx)
	{
		globalOutputStream() << getName() << "::initialiseModule called." << std::endl;

		GlobalCommandSystem().addCommand("ReadableEditorDialog", ui::ReadableEditorDialog::RunDialog);
		GlobalEventManager().addCommand("ReadableEditorDialog", "ReadableEditorDialog");

		GlobalUIManager().getMenuManager().add("main/entity",
			"ReadableEditorDialog", ui::menuItem, 
			"Readable Editor", // caption
			"", // icon
			"ReadableEditorDialog"
		);
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
