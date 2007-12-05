#ifndef IGRID_H_
#define IGRID_H_

/* greebo: The interface of the grid system
 * 
 * Use these methods to set/get the grid size of the xyviews
 */

#include "imodule.h"
#include "signal/signalfwd.h"

enum GridSize {
	GRID_0125 = -3,
	GRID_025 = -2,
	GRID_05 = -1,
	GRID_1 = 0,
	GRID_2 = 1,
	GRID_4 = 2,
	GRID_8 = 3,
	GRID_16 = 4,
	GRID_32 = 5,
	GRID_64 = 6,
	GRID_128 = 7,
	GRID_256 = 8,
};

const std::string MODULE_GRID("Grid");

class IGridManager :
	public RegisterableModule
{
public:
	virtual void setGridSize(GridSize gridSize) = 0;
	virtual float getGridSize() const = 0;
	
	virtual int getGridPower() const = 0;
	
	virtual void gridDown() = 0;
	virtual void gridUp() = 0;
	
	virtual void addGridChangeCallback(const SignalHandler& handler) = 0;
	
	virtual void gridChangeNotify() = 0;
}; // class IGridManager

// This is the accessor for the grid module
inline IGridManager& GlobalGrid() {
	// Cache the reference locally
	static IGridManager& _grid(
		*boost::static_pointer_cast<IGridManager>(
			module::GlobalModuleRegistry().getModule(MODULE_GRID)
		)
	);
	return _grid;
}

#endif /*IGRID_H_*/
