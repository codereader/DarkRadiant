#include "WaveFrontModule.h"

#include "itextstream.h"
#include "icommandsystem.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "iradiant.h"
#include "iregistry.h"
#include "selectionlib.h"
#include <boost/bind.hpp>

#include "WaveFrontExporter.h"

namespace exporter
{

void WaveFrontModule::exportSelectionAsOBJ(const cmd::ArgumentList& args)
{
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	if (info.totalCount == 0)
	{
		globalErrorStream() << "Nothing selected, cannot export." << std::endl;
		return;
	}
	
	// Query the filename from the user
	ui::IFileChooserPtr chooser = GlobalRadiant().createFileChooser("Save as Obj", false, "*.obj", ".obj");

	chooser->setCurrentPath(GlobalRegistry().get(RKEY_MAP_PATH));

	std::string path = chooser->display();

	if (!path.empty())
	{
		globalOutputStream() << "Exporting selection as OBJ to " << path << std::endl;

		// Instantiate a new exporter
		WaveFrontExporter exporter(path);
		exporter.exportSelection();
	}
}

// RegisterableModule implementation
const std::string& WaveFrontModule::getName() const
{
	static std::string _name("WaveFrontExporter");
	return _name;
}

const StringSet& WaveFrontModule::getDependencies() const
{
	static StringSet _dependencies; 

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_COMMANDSYSTEM);
		_dependencies.insert(MODULE_RADIANT);
		_dependencies.insert(MODULE_SELECTIONSYSTEM);
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_UIMANAGER);
		_dependencies.insert(MODULE_EVENTMANAGER);
	}

	return _dependencies;
}

void WaveFrontModule::initialiseModule(const ApplicationContext& ctx)
{
	globalOutputStream() << "WaveFrontModule::initialiseModule called" << std::endl;

	// Register the command
	GlobalCommandSystem().addCommand(
		"ExportSelectedAsOBJ", 
		boost::bind(&WaveFrontModule::exportSelectionAsOBJ, this, _1)
	);

	// Bind the reloadscripts command to the menu
	GlobalEventManager().addCommand("ExportSelectedAsOBJ", "ExportSelectedAsOBJ");

	// Add the menu item
	IMenuManager& mm = GlobalUIManager().getMenuManager();
	mm.insert("main/file/createCM", 	// menu location path
			"ExportSelectedAsOBJ", // name
			ui::menuItem,	// type
			"Export Selection as OBJ...",	// caption
			"",	// icon
			"ExportSelectedAsOBJ"); // event name
}

} // namespace
