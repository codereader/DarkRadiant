#pragma once

#include "imodule.h"
#include "igrid.h"
#include "ieventmanager.h"
#include <map>
#include <sigc++/connection.h>

namespace ui
{

/**
 * Covering the grid functionality in the UI.
 */
class GridUserInterface :
	public RegisterableModule
{
private:
	sigc::connection _gridChangedConn;

	std::map<GridSize, std::string> _toggleItemNames;

public:
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const ApplicationContext& ctx) override;
	void shutdownModule() override;

private:
	void toggleGrid(GridSize size, bool newState);
	void onGridChanged();
};

}