#ifndef IGRID_H_
#define IGRID_H_

/* greebo: The interface of the grid system
 * 
 * Use these methods to set/get the grid size of the xyviews
 */

#include "generic/constant.h"
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

class IGridManager
{
public:
	INTEGER_CONSTANT(Version, 1);
	STRING_CONSTANT(Name, "grid");

	virtual void setGridSize(GridSize gridSize) = 0;
	virtual float getGridSize() const = 0;
	
	virtual int getGridPower() const = 0;
	
	virtual void gridDown() = 0;
	virtual void gridUp() = 0;
	
	virtual void addGridChangeCallback(const SignalHandler& handler) = 0;
	
	virtual void gridChangeNotify() = 0;
}; // class IGridManager

// Module definitions

#include "modulesystem.h"

template<typename Type>
class GlobalModule;
typedef GlobalModule<IGridManager> GlobalGridModule;

template<typename Type>
class GlobalModuleRef;
typedef GlobalModuleRef<IGridManager> GlobalGridModuleRef;

// This is the accessor for the grid module
inline IGridManager& GlobalGrid() {
	return GlobalGridModule::getTable();
}

#endif /*IGRID_H_*/
