#pragma once

#include "GridItem.h"
#include "icommandsystem.h"
#include "ipreferencesystem.h"
#include <list>

namespace ui
{

class GridManager :
	public IGridManager
{
private:
	typedef std::pair<const std::string, GridItem> NamedGridItem;
	typedef std::list<NamedGridItem> GridItems;

	GridItems _gridItems;

	// The currently active grid size
	GridSize _activeGridSize;

	sigc::signal<void> _sigGridChanged;

public:
	GridManager();

	sigc::signal<void> signal_gridChanged() const override;

	void gridUp() override;
	void gridDown() override;

	void setGridSize(GridSize gridSize) override;
	float getGridSize() const override;

	int getGridPower() const override;

	GridLook getMajorLook() const override;
	GridLook getMinorLook() const override;

public:
	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const ApplicationContext& ctx) override;
	void shutdownModule() override;

private:
	void gridChangeNotify();
	void gridChanged();

	void loadDefaultValue();

	void populateGridItems();

	void registerCommands();

	ComboBoxValueList getGridList();

	void constructPreferences();

	void gridUpCmd(const cmd::ArgumentList& args);
	void gridDownCmd(const cmd::ArgumentList& args);

	static GridLook getLookFromNumber(int i);
};

}
