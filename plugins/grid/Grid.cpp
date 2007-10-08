#include "igrid.h"

#include <iostream>
#include <map>
#include "imodule.h"
#include "iradiant.h"
#include "ieventmanager.h"
#include "iregistry.h"
#include "ipreferencesystem.h"
#include "signal/signal.h"

#include "GridItem.h"

	namespace {
		const std::string RKEY_DEFAULT_GRID_SIZE = "user/ui/grid/defaultGridPower";
	}

class GridManager :
	public IGridManager,
	public RegistryKeyObserver
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
			_dependencies.insert(MODULE_RADIANT);
			_dependencies.insert(MODULE_XMLREGISTRY);
			_dependencies.insert(MODULE_EVENTMANAGER);
			_dependencies.insert(MODULE_PREFERENCESYSTEM);
		}

		return _dependencies;
	}
	
	virtual void initialiseModule(const ApplicationContext& ctx) {
		globalOutputStream() << "GridManager::initialiseModule called.\n";
		
		populateGridItems();
		registerCommands();
		
		// Connect self to the according registry keys
		GlobalRegistry().addKeyObserver(this, RKEY_DEFAULT_GRID_SIZE);
		
		// Load the default value from the registry
		keyChanged();
	}

private:
	typedef std::map<const std::string, GridItem> GridItemMap;
	
	GridItemMap _gridItems;
	
	// The currently active grid size
	GridSize _activeGridSize;
	
	Signal0 _gridChangeCallbacks;
	
public:
	GridManager() :
		_activeGridSize(GRID_8) 
	{}
	
	void keyChanged() {
		// Get the registry value
		int registryValue = GlobalRegistry().getInt(RKEY_DEFAULT_GRID_SIZE);
		
		// Constrain the values to the allowed ones 
		if (registryValue < GRID_0125) {
			registryValue = static_cast<int>(GRID_0125);
		}
		
		if (registryValue > GRID_256) {
			registryValue = static_cast<int>(GRID_256);
		}
		
		_activeGridSize = static_cast<GridSize>(registryValue);
		
		// Notify the world about the grid change
		gridChangeNotify();
	}
	
	void populateGridItems() {
		// Populate the GridItem map
		_gridItems.insert(GridItemMap::value_type("0.125", GridItem(GRID_0125, *this)));
		_gridItems.insert(GridItemMap::value_type("0.25", GridItem(GRID_025, *this)));
		_gridItems.insert(GridItemMap::value_type("0.5", GridItem(GRID_05, *this)));
		_gridItems.insert(GridItemMap::value_type("1", GridItem(GRID_1, *this)));
		_gridItems.insert(GridItemMap::value_type("2", GridItem(GRID_2, *this)));
		_gridItems.insert(GridItemMap::value_type("4", GridItem(GRID_4, *this)));
		_gridItems.insert(GridItemMap::value_type("8", GridItem(GRID_8, *this)));
		_gridItems.insert(GridItemMap::value_type("16", GridItem(GRID_16, *this)));
		_gridItems.insert(GridItemMap::value_type("32", GridItem(GRID_32, *this)));
		_gridItems.insert(GridItemMap::value_type("64", GridItem(GRID_64, *this)));
		_gridItems.insert(GridItemMap::value_type("128", GridItem(GRID_128, *this)));
		_gridItems.insert(GridItemMap::value_type("256", GridItem(GRID_256, *this)));
	}
	
	void registerCommands() {
		for (GridItemMap::iterator i = _gridItems.begin(); i != _gridItems.end(); i++) {
			std::string toggleName = "SetGrid";
			toggleName += i->first; // Makes "SetGrid" to "SetGrid64", for example
			GridItem& gridItem = i->second;
			
			GlobalEventManager().addToggle(toggleName, GridItem::ActivateCaller(gridItem)); 
		}
		
		GlobalEventManager().addCommand("GridDown", MemberCaller<GridManager, &GridManager::gridDown>(*this));
		GlobalEventManager().addCommand("GridUp", MemberCaller<GridManager, &GridManager::gridUp>(*this));
	}
	
	ComboBoxValueList getGridList() {
		ComboBoxValueList returnValue;
		
		for (GridItemMap::iterator i = _gridItems.begin(); i != _gridItems.end(); i++) {
			returnValue.push_back(i->first);
		}
		
		return returnValue;
	}
	
	void constructPreferences() {
		/*PreferencesPage* page(group.createPage("Grid", "Default Grid Settings"));
	
		page->appendCombo("Default Grid Size", RKEY_DEFAULT_GRID_SIZE, getGridList());*/
	}
	
	void addGridChangeCallback(const SignalHandler& handler) {
		_gridChangeCallbacks.connectLast(handler);
		handler();
	}
	
	void gridChangeNotify() {
		_gridChangeCallbacks();
	}
	
	void gridDown() {
		if (_activeGridSize > GRID_0125) {
			int _activeGridIndex = static_cast<int>(_activeGridSize);
			_activeGridIndex--;
			setGridSize(static_cast<GridSize>(_activeGridIndex));
		}
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
		for (GridItemMap::iterator i = _gridItems.begin(); i != _gridItems.end(); i++) {
			std::string toggleName = "SetGrid";
			toggleName += i->first; // Makes "SetGrid" to "SetGrid64", for example
			GridItem& gridItem = i->second;
			
			GlobalEventManager().setToggled(toggleName, _activeGridSize == gridItem.getGridSize()); 
		}
		
		gridChangeNotify();
	}

}; // class GridManager
typedef boost::shared_ptr<GridManager> GridManagerPtr;

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(GridManagerPtr(new GridManager));
	
	// Initialise the streams
	const ApplicationContext& ctx = registry.getApplicationContext();
	GlobalOutputStream::instance().setOutputStream(ctx.getOutputStream());
	GlobalErrorStream::instance().setOutputStream(ctx.getOutputStream());
	
	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);
}
