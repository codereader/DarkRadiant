#include "GridInterface.h"

#include <pybind11/pybind11.h>

#include "itextstream.h"
#include "igrid.h"

namespace script 
{

void GridInterface::setGridSize(int gridSize)
{
	// Sanity-check the incoming int
	if (gridSize < GRID_0125 || gridSize > GRID_256) {
		rError() << "Invalid grid size passed, allowed values are in the range "
			<< "[" << GRID_0125 << ".." << GRID_256 << "]" << std::endl;
		return;
	}

	GlobalGrid().setGridSize(static_cast<GridSize>(gridSize));
}

float GridInterface::getGridSize()
{
	return GlobalGrid().getGridSize();
}

int GridInterface::getGridPower()
{
	return GlobalGrid().getGridPower();
}

void GridInterface::gridDown() 
{
	GlobalGrid().gridDown();
}

void GridInterface::gridUp()
{
	GlobalGrid().gridUp();
}

void GridInterface::registerInterface(py::module& scope, py::dict& globals)
{
	// Add the module declaration to the given python namespace
	py::class_<GridInterface> grid(scope, "Grid");

	grid.def("setGridSize", &GridInterface::setGridSize);
	grid.def("getGridSize", &GridInterface::getGridSize);
	grid.def("getGridPower", &GridInterface::getGridPower);
	grid.def("gridDown", &GridInterface::gridDown);
	grid.def("gridUp", &GridInterface::gridUp);

	// Now point the Python variable "GlobalGrid" to this instance
	globals["GlobalGrid"] = this;
}

} // namespace script
