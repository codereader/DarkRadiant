#include "WaveFrontModule.h"

#include "itextstream.h"
#include "icommandsystem.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "ifilechooser.h"
#include "idialogmanager.h"
#include "iregistry.h"
#include "selectionlib.h"
#include <boost/bind.hpp>
#include "i18n.h"

#include "WaveFrontExporter.h"

namespace exporter
{

void WaveFrontModule::exportSelectionAsOBJ(const cmd::ArgumentList& args)
{
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	if (info.totalCount == 0)
	{
		rError() << "Nothing selected, cannot export." << std::endl;
		return;
	}

	// Query the filename from the user
	ui::IFileChooserPtr chooser = GlobalDialogManager().createFileChooser(
		_("Save as Obj"), false, false, "*.obj", ".obj"
	);

	chooser->setCurrentPath(GlobalRegistry().get(RKEY_MAP_PATH));

	std::string path = chooser->display();

	if (!path.empty())
	{
		rMessage() << "Exporting selection as OBJ to " << path << std::endl;

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
		_dependencies.insert(MODULE_SELECTIONSYSTEM);
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_UIMANAGER);
		_dependencies.insert(MODULE_EVENTMANAGER);
	}

	return _dependencies;
}

void WaveFrontModule::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << "WaveFrontModule::initialiseModule called" << std::endl;

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
			_("Export Selection as OBJ..."),	// caption
			"",	// icon
			"ExportSelectedAsOBJ"); // event name
}

} // namespace
