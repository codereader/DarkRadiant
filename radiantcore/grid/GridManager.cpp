#include "GridManager.h"

#include <iostream>
#include <map>
#include "itextstream.h"
#include "debugging/debugging.h"
#include "imodule.h"
#include "icommandsystem.h"
#include "ipreferencesystem.h"
#include "string/string.h"

#include "registry/registry.h"
#include "i18n.h"
#include "GridItem.h"
#include <functional>
#include <fmt/format.h>

#include "module/StaticModule.h"

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
		_dependencies.insert(MODULE_COMMANDSYSTEM);
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
	}

	return _dependencies;
}

void GridManager::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << "GridManager::initialiseModule called.\n";

	populateGridItems();
	registerCommands();

	constructPreferences();

	// Load the default value from the registry
	loadDefaultValue();
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
	for (int size = GRID_0125; size <= GRID_256; size++)
	{
		_gridItems.emplace_back(
			grid::getStringForSize(static_cast<GridSize>(size)),
			GridItem(static_cast<GridSize>(size), *this)
		);
	}
}

void GridManager::registerCommands()
{
	GlobalCommandSystem().addCommand("SetGrid",
		std::bind(&GridManager::setGridCmd, this, std::placeholders::_1),
		{ cmd::ARGTYPE_STRING });

	// Grid size shortcuts are defined in commandsystem.xml like "SetGrid4" => "SetGrid 16"
	// Create shortcut statements that can accept accelerator bindings

	GlobalCommandSystem().addCommand("GridDown", std::bind(&GridManager::gridDownCmd, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("GridUp", std::bind(&GridManager::gridUpCmd, this, std::placeholders::_1));
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

void GridManager::setGridCmd(const cmd::ArgumentList& args)
{
	if (args.size() != 1)
	{
		rError() << "Usage: SetGrid [";

		for (const auto& item : _gridItems)
		{
			rError() << item.first << "|";
		}

		rError() << "]" << std::endl;
		return;
	}

	// Look up the grid item by the argument string
	auto gridStr = args[0].getString();

	for (const auto& item : _gridItems)
	{
		if (item.first == gridStr)
		{
			setGridSize(item.second.getGridSize());
			return;
		}
	}

	rError() << "Unknown grid size: " << gridStr << std::endl;
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
    if (_activeGridSize != gridSize)
    {
        _activeGridSize = gridSize;
        gridChangeNotify();
    }
}

float GridManager::getGridSize(grid::Space space) const
{
	return pow(2.0f, static_cast<float>(getGridPower(space)));
}

int GridManager::getGridPower(grid::Space space) const
{
    int power = static_cast<int>(_activeGridSize);

    // Texture space is using smaller grid sizes, since it doesn't make much
    // sense to have grid spacing greater than 1.0
    if (space == grid::Space::Texture)
    {
        power -= 7;

        if (power < -10) power = -10;
        if (power > 0) power = 0;
    }

    return power;
}

int GridManager::getGridBase(grid::Space space) const
{
    return 2;
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

module::StaticModuleRegistration<GridManager> staticGridManagerModule;

}
