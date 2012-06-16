#include "igrid.h"

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
#include <boost/bind.hpp>

	namespace {
		const std::string RKEY_DEFAULT_GRID_SIZE = "user/ui/grid/defaultGridPower";
		const std::string RKEY_GRID_LOOK_MAJOR = "user/ui/grid/majorGridLook";
		const std::string RKEY_GRID_LOOK_MINOR = "user/ui/grid/minorGridLook";
	}

class GridManager :
	public IGridManager
{
public:
	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name(MODULE_GRID);
		return _name;
	}

	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies;

		if (_dependencies.empty()) {
			_dependencies.insert(MODULE_XMLREGISTRY);
			_dependencies.insert(MODULE_EVENTMANAGER);
			_dependencies.insert(MODULE_COMMANDSYSTEM);
			_dependencies.insert(MODULE_PREFERENCESYSTEM);
			_dependencies.insert(MODULE_UIMANAGER);
			_dependencies.insert(MODULE_MAINFRAME);
		}

		return _dependencies;
	}

	virtual void initialiseModule(const ApplicationContext& ctx) {
		rMessage() << "GridManager::initialiseModule called.\n";

		// Add the grid status bar element
		GlobalUIManager().getStatusBarManager().addTextElement("GridStatus", "grid_up.png", IStatusBarManager::POS_GRID);
		GlobalUIManager().getStatusBarManager().setText("GridStatus", "-");

		populateGridItems();
		registerCommands();

		constructPreferences();

		// Load the default value from the registry
		loadDefaultValue();

		// Update the Toggle item status
		gridChanged();
	}

	virtual void shutdownModule() {
		// Map the [GRID_0125...GRID_256] values (starting from -3) to [0..N]
		int registryValue = static_cast<int>(_activeGridSize) - static_cast<int>(GRID_0125);

		registry::setValue(RKEY_DEFAULT_GRID_SIZE, registryValue);
	}

private:
	typedef std::list< std::pair<const std::string, GridItem> > GridItems;

	GridItems _gridItems;

	// The currently active grid size
	GridSize _activeGridSize;

    sigc::signal<void> _sigGridChanged;

public:
	GridManager() :
		_activeGridSize(GRID_8)
	{}

	void loadDefaultValue() {
		// Get the registry value
		int registryValue = registry::getValue<int>(RKEY_DEFAULT_GRID_SIZE);

		// Map the [0..N] values to [GRID_0125...GRID_256]
		int mapped = registryValue + static_cast<int>(GRID_0125);

		if (mapped >= GRID_0125 && mapped <= GRID_256) {
			_activeGridSize = static_cast<GridSize>(mapped);
		}
		else {
			_activeGridSize = GRID_8;
		}
	}

	void populateGridItems() {
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

	void registerCommands() {
		for (GridItems::iterator i = _gridItems.begin(); i != _gridItems.end(); i++) {
			std::string toggleName = "SetGrid";
			toggleName += i->first; // Makes "SetGrid" to "SetGrid64", for example
			GridItem& gridItem = i->second;

			GlobalEventManager().addToggle(toggleName,
				boost::bind(&GridItem::activate, &gridItem, _1));
		}

		GlobalCommandSystem().addCommand("GridDown", boost::bind(&GridManager::gridDownCmd, this, _1));
		GlobalCommandSystem().addCommand("GridUp", boost::bind(&GridManager::gridUpCmd, this, _1));

		GlobalEventManager().addCommand("GridDown", "GridDown");
		GlobalEventManager().addCommand("GridUp", "GridUp");
	}

	ComboBoxValueList getGridList() {
		ComboBoxValueList returnValue;

		for (GridItems::iterator i = _gridItems.begin(); i != _gridItems.end(); i++) {
			returnValue.push_back(i->first);
		}

		return returnValue;
	}

	void constructPreferences() {
		PreferencesPagePtr page = GlobalPreferenceSystem().getPage(_("Settings/Grid"));

		page->appendCombo(_("Default Grid Size"), RKEY_DEFAULT_GRID_SIZE, getGridList());

		ComboBoxValueList looks;

		looks.push_back(_("Lines"));
		looks.push_back(_("Dotted Lines"));
		looks.push_back(_("More Dotted Lines"));
		looks.push_back(_("Crosses"));
		looks.push_back(_("Dots"));
		looks.push_back(_("Big Dots"));
		looks.push_back(_("Squares"));

		page->appendCombo(_("Major Grid Style"), RKEY_GRID_LOOK_MAJOR, looks);
		page->appendCombo(_("Minor Grid Style"), RKEY_GRID_LOOK_MINOR, looks);
	}


	sigc::signal<void> signal_gridChanged() const
    {
        return _sigGridChanged;
    }

	void gridChangeNotify() 
    {
		_sigGridChanged();
	}

	void gridDownCmd(const cmd::ArgumentList& args) {
		gridDown();
	}

	void gridDown() {
		if (_activeGridSize > GRID_0125) {
			int _activeGridIndex = static_cast<int>(_activeGridSize);
			_activeGridIndex--;
			setGridSize(static_cast<GridSize>(_activeGridIndex));
		}
	}

	void gridUpCmd(const cmd::ArgumentList& args) {
		gridUp();
	}

	void gridUp() {
		if (_activeGridSize < GRID_256) {
			int _activeGridIndex = static_cast<int>(_activeGridSize);
			_activeGridIndex++;
			setGridSize(static_cast<GridSize>(_activeGridIndex));
		}
	}

	void setGridSize(GridSize gridSize) {
		_activeGridSize = gridSize;

		gridChanged();
	}

	float getGridSize() const {
		return pow(2.0f, static_cast<int>(_activeGridSize));
	}

	int getGridPower() const {
		return static_cast<int>(_activeGridSize);
	}

	void gridChanged() {
		for (GridItems::iterator i = _gridItems.begin(); i != _gridItems.end(); ++i) {
			std::string toggleName = "SetGrid";
			toggleName += i->first; // Makes "SetGrid" to "SetGrid64", for example
			GridItem& gridItem = i->second;

			GlobalEventManager().setToggled(toggleName, _activeGridSize == gridItem.getGridSize());
		}

		GlobalUIManager().getStatusBarManager().setText("GridStatus", string::to_string(getGridSize()));

		gridChangeNotify();

		GlobalMainFrame().updateAllWindows();
	}

	static GridLook getLookFromNumber(int i)
	{
		if (i >= GRIDLOOK_LINES && i <= GRIDLOOK_SQUARES)
		{
			return static_cast<GridLook>(i);
		}

		return GRIDLOOK_LINES;
	}

	GridLook getMajorLook() const {
		return getLookFromNumber( registry::getValue<int>(RKEY_GRID_LOOK_MAJOR) );
	}

	GridLook getMinorLook() const {
		return getLookFromNumber( registry::getValue<int>(RKEY_GRID_LOOK_MINOR) );
	}

}; // class GridManager
typedef boost::shared_ptr<GridManager> GridManagerPtr;

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) 
{
	registry.registerModule(GridManagerPtr(new GridManager));

	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());

	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);

	// Set up the assertion handler
	GlobalErrorHandler() = registry.getApplicationContext().getErrorHandlingFunction();
}
