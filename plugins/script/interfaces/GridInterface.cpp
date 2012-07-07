#include "GridInterface.h"

#include "itextstream.h"
#include "igrid.h"

namespace script {

void GridInterface::setGridSize(int gridSize) {
	// Sanity-check the incoming int
	if (gridSize < GRID_0125 || gridSize > GRID_256) {
		rError() << "Invalid grid size passed, allowed values are in the range "
			<< "[" << GRID_0125 << ".." << GRID_256 << "]" << std::endl;
		return;
	}

	GlobalGrid().setGridSize(static_cast<GridSize>(gridSize));
}

float GridInterface::getGridSize() {
	return GlobalGrid().getGridSize();
}

int GridInterface::getGridPower() {
	return GlobalGrid().getGridPower();
}

void GridInterface::gridDown() {
	GlobalGrid().gridDown();
}

void GridInterface::gridUp()  {
	GlobalGrid().gridUp();
}

void GridInterface::registerInterface(boost::python::object& nspace) {
	// Add the module declaration to the given python namespace
	nspace["GlobalGrid"] = boost::python::class_<GridInterface>("GlobalGrid")
		.def("setGridSize", &GridInterface::setGridSize)
		.def("getGridSize", &GridInterface::getGridSize)
		.def("getGridPower", &GridInterface::getGridPower)
		.def("gridDown", &GridInterface::gridDown)
		.def("gridUp", &GridInterface::gridUp)
	;

	// Now point the Python variable "GlobalGrid" to this instance
	nspace["GlobalGrid"] = boost::python::ptr(this);
}

} // namespace script
