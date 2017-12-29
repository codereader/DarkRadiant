#include "GridManager.h"

#include <iostream>
#include <map>
#include "itextstream.h"
#include "debugging/debugging.h"
#include "imodule.h"
#include "icommandsystem.h"
#include "imainframe.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "ipreferencesystem.h"
#include "string/string.h"

#include "registry/registry.h"
#include "i18n.h"
#include "GridItem.h"
#include <functional>
#include <fmt/format.h>

#include "modulesystem/StaticModule.h"

namespace ui
{

namespace
{
	const char* const RKEY_DEFAULT_GRID_SIZE = "user/ui/grid/defaultGridPower";
	const char* const RKEY_GRID_LOOK_MAJOR = "user/ui/grid/majorGridLook";
	const char* const RKEY_GRID_LOOK_MINOR = "user/ui/grid/minorGridLook";
}

GridManager::GridManager() :
	_activeGridSize(GRID_8)
{}

const std::string& GridManager::getName() const
{
	static std::string _name(MODULE_GRID);
	return _name;
}

const StringSet& GridManager::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_COMMANDSYSTEM);
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
		_dependencies.insert(MODULE_UIMANAGER);
		_dependencies.insert(MODULE_MAINFRAME);
	}

	return _dependencies;
}

void GridManager::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << "GridManager::initialiseModule called.\n";

	// Add the grid status bar element
	GlobalUIManager().getStatusBarManager().addTextElement("GridStatus", "grid_up.png", IStatusBarManager::POS_GRID, _("Current Grid Size"));
	GlobalUIManager().getStatusBarManager().setText("GridStatus", "-");

	populateGridItems();
	registerCommands();

	constructPreferences();

	// Load the default value from the registry
	loadDefaultValue();

	// Update the Toggle item status
	gridChanged();
}

void GridManager::shutdownModule()
{
	// Map the [GRID_0125...GRID_256] values (starting from -3) to [0..N]
	int registryValue = static_cast<int>(_activeGridSize) - static_cast<int>(GRID_0125);

	registry::setValue(RKEY_DEFAULT_GRID_SIZE, registryValue);
}

void GridManager::loadDefaultValue()
{
	// Get the registry value
	int registryValue = registry::getValue<int>(RKEY_DEFAULT_GRID_SIZE);

	// Map the [0..N] values to [GRID_0125...GRID_256]
	int mapped = registryValue + static_cast<int>(GRID_0125);

	if (mapped >= GRID_0125 && mapped <= GRID_256)
	{
		_activeGridSize = static_cast<GridSize>(mapped);
	}
	else
	{
		_activeGridSize = GRID_8;
	}
}

void GridManager::populateGridItems()
{
	// Populate the GridItem map
	_gridItems.push_back(GridItems::value_type("0.125", GridItem(GRID_0125, *this)));
	_gridItems.push_back(GridItems::value_type("0.25", GridItem(GRID_025, *this)));
	_gridItems.push_back(GridItems::value_type("0.5", GridItem(GRID_05, *this)));
	_gridItems.push_back(GridItems::value_type("1", GridItem(GRID_1, *this)));
	_gridItems.push_back(GridItems::value_type("2", GridItem(GRID_2, *this)));
	_gridItems.push_back(GridItems::value_type("4", GridItem(GRID_4, *this)));
	_gridItems.push_back(GridItems::value_type("8", GridItem(GRID_8, *this)));
	_gridItems.push_back(GridItems::value_type("16", GridItem(GRID_16, *this)));
	_gridItems.push_back(GridItems::value_type("32", GridItem(GRID_32, *this)));
	_gridItems.push_back(GridItems::value_type("64", GridItem(GRID_64, *this)));
	_gridItems.push_back(GridItems::value_type("128", GridItem(GRID_128, *this)));
	_gridItems.push_back(GridItems::value_type("256", GridItem(GRID_256, *this)));
}

void GridManager::registerCommands()
{
	for (NamedGridItem& i : _gridItems)
	{
		std::string toggleName = "SetGrid";
		toggleName += i.first; // Makes "SetGrid" to "SetGrid64", for example
		GridItem& gridItem = i.second;

		GlobalEventManager().addToggle(toggleName,
			std::bind(&GridItem::activate, &gridItem, std::placeholders::_1));
	}

	GlobalCommandSystem().addCommand("GridDown", std::bind(&GridManager::gridDownCmd, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("GridUp", std::bind(&GridManager::gridUpCmd, this, std::placeholders::_1));

	GlobalEventManager().addCommand("GridDown", "GridDown");
	GlobalEventManager().addCommand("GridUp", "GridUp");
}

ComboBoxValueList GridManager::getGridList()
{
	ComboBoxValueList returnValue;

	for (const NamedGridItem& i : _gridItems)
	{
		returnValue.push_back(i.first);
	}

	return returnValue;
}

void GridManager::constructPreferences()
{
	IPreferencePage& page = GlobalPreferenceSystem().getPage(_("Settings/Grid"));

	page.appendCombo(_("Default Grid Size"), RKEY_DEFAULT_GRID_SIZE, getGridList());

	ComboBoxValueList looks;

	looks.push_back(_("Lines"));
	looks.push_back(_("Dotted Lines"));
	looks.push_back(_("More Dotted Lines"));
	looks.push_back(_("Crosses"));
	looks.push_back(_("Dots"));
	looks.push_back(_("Big Dots"));
	looks.push_back(_("Squares"));

	page.appendCombo(_("Major Grid Style"), RKEY_GRID_LOOK_MAJOR, looks);
	page.appendCombo(_("Minor Grid Style"), RKEY_GRID_LOOK_MINOR, looks);
}


sigc::signal<void> GridManager::signal_gridChanged() const
{
	return _sigGridChanged;
}

void GridManager::gridChangeNotify()
{
	_sigGridChanged();
}

void GridManager::gridDownCmd(const cmd::ArgumentList& args) 
{
	gridDown();
}

void GridManager::gridDown()
{
	if (_activeGridSize > GRID_0125)
	{
		int _activeGridIndex = static_cast<int>(_activeGridSize);
		_activeGridIndex--;
		setGridSize(static_cast<GridSize>(_activeGridIndex));
	}
}

void GridManager::gridUpCmd(const cmd::ArgumentList& args)
{
	gridUp();
}

void GridManager::gridUp()
{
	if (_activeGridSize < GRID_256) 
	{
		int _activeGridIndex = static_cast<int>(_activeGridSize);
		_activeGridIndex++;
		setGridSize(static_cast<GridSize>(_activeGridIndex));
	}
}

void GridManager::setGridSize(GridSize gridSize) 
{
	_activeGridSize = gridSize;

	gridChanged();
}

float GridManager::getGridSize() const
{
	return pow(2.0f, static_cast<int>(_activeGridSize));
}

int GridManager::getGridPower() const 
{
	return static_cast<int>(_activeGridSize);
}

void GridManager::gridChanged()
{
	for (const NamedGridItem& i : _gridItems)
	{
		std::string toggleName = "SetGrid";
		toggleName += i.first; // Makes "SetGrid" to "SetGrid64", for example
		const GridItem& gridItem = i.second;

		GlobalEventManager().setToggled(toggleName, _activeGridSize == gridItem.getGridSize());
	}

	GlobalUIManager().getStatusBarManager().setText("GridStatus", fmt::format("{0:g}", getGridSize()));

	gridChangeNotify();

	GlobalMainFrame().updateAllWindows();
}

GridLook GridManager::getLookFromNumber(int i)
{
	if (i >= GRIDLOOK_LINES && i <= GRIDLOOK_SQUARES)
	{
		return static_cast<GridLook>(i);
	}

	return GRIDLOOK_LINES;
}

GridLook GridManager::getMajorLook() const
{
	return getLookFromNumber(registry::getValue<int>(RKEY_GRID_LOOK_MAJOR));
}

GridLook GridManager::getMinorLook() const
{
	return getLookFromNumber(registry::getValue<int>(RKEY_GRID_LOOK_MINOR));
}

module::StaticModule<GridManager> staticGridManagerModule;

}
