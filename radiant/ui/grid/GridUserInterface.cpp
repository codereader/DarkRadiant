#include "GridUserInterface.h"

#include <functional>
#include "iuimanager.h"
#include "imainframe.h"

#include "module/StaticModule.h"

namespace ui
{

const std::string& GridUserInterface::getName() const
{
	static std::string _name("GridUserInterface");
	return _name;
}

const StringSet& GridUserInterface::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_GRID);
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_UIMANAGER);
	}

	return _dependencies;
}

void GridUserInterface::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	// Add the grid status bar element
	GlobalUIManager().getStatusBarManager().addTextElement("GridStatus", "grid_up.png", 
		IStatusBarManager::POS_GRID, _("Current Grid Size"));
	GlobalUIManager().getStatusBarManager().setText("GridStatus", "-");

	_gridChangedConn = GlobalGrid().signal_gridChanged().connect(
		std::bind(&GridUserInterface::onGridChanged, this)
	);

	// Add a Toggle element for each grid size, such that the Menu items can bind to it
	for (int size = GRID_0125; size <= GRID_256; size++)
	{
		GridSize gridSize = static_cast<GridSize>(size);

		std::string toggleName = std::string("SetGrid") + grid::getStringForSize(gridSize);
		auto toggle = GlobalEventManager().addToggle(toggleName,
			std::bind(&GridUserInterface::toggleGrid, this, gridSize, std::placeholders::_1));

		_toggleItemNames.emplace(gridSize, toggleName);

		GlobalEventManager().setToggled(toggleName, GlobalGrid().getGridPower() == size);
	}
}

void GridUserInterface::shutdownModule()
{
	_gridChangedConn.disconnect();
}

void GridUserInterface::onGridChanged()
{
	for (const auto& item : _toggleItemNames)
	{
		GlobalEventManager().setToggled(item.second, GlobalGrid().getGridPower() == item.first);
	}

	GlobalUIManager().getStatusBarManager().setText("GridStatus", 
		fmt::format("{0:g}", GlobalGrid().getGridSize()));

	GlobalMainFrame().updateAllWindows();
}

void GridUserInterface::toggleGrid(GridSize size, bool newState)
{
	GlobalGrid().setGridSize(size);
}

module::StaticModule<GridUserInterface> gridUiModule;

}
