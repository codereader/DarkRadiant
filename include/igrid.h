#ifndef IGRID_H_
#define IGRID_H_

/* greebo: The interface of the grid system
 *
 * Use these methods to set/get the grid size of the xyviews
 */

#include "imodule.h"
#include <sigc++/signal.h>

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


// grid renderings
enum GridLook {
	GRIDLOOK_LINES,
	GRIDLOOK_DOTLINES,
	GRIDLOOK_MOREDOTLINES,
	GRIDLOOK_CROSSES,
	GRIDLOOK_DOTS,
	GRIDLOOK_BIGDOTS,
	GRIDLOOK_SQUARES,
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

	virtual GridLook getMajorLook() const = 0;
	virtual GridLook getMinorLook() const = 0;

    /// Signal emitted when the grid is changed
	virtual sigc::signal<void> signal_gridChanged() const = 0;

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
