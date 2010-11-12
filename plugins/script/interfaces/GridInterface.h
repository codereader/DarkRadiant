#ifndef _GRID_INTERFACE_H_
#define _GRID_INTERFACE_H_

#include <boost/python.hpp>
#include "iscript.h"

namespace script {

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
	void registerInterface(boost::python::object& nspace);
};
typedef boost::shared_ptr<GridInterface> GridInterfacePtr;

} // namespace script

#endif /* _GRID_INTERFACE_H_ */
