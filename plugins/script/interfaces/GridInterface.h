#pragma once

#include "iscript.h"

namespace script 
{

/**
 * greebo: This class provides the script interface for the GlobalGrid module.
 */
class GridInterface :
	public IScriptInterface
{
public:
	// Wrapped methods
	void setGridSize(int gridSize);
	float getGridSize();
	int getGridPower();
	void gridDown();
	void gridUp();

	// IScriptInterface implementation
	void registerInterface(py::module& scope, py::dict& globals) override;
};

} // namespace script
