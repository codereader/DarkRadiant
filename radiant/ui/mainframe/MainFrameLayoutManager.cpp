#include "MainFrameLayoutManager.h"

#include "itextstream.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "imainframe.h"
#include "icommandsystem.h"

#include "modulesystem/StaticModule.h"

#include "FloatingLayout.h"
#include "SplitPaneLayout.h"
#include "RegularLayout.h"
#include "EmbeddedLayout.h"

namespace ui {

IMainFrameLayoutPtr MainFrameLayoutManager::getLayout(const std::string& name) {
	// Try to lookup that layout
	LayoutMap::const_iterator found = _layouts.find(name);

	if (found == _layouts.end()) {
		rError() << "MainFrameLayoutManager: Could not find " << name << std::endl;
		return IMainFrameLayoutPtr();
	}

	// Call the creation function and retrieve the layout
	IMainFrameLayoutPtr layout = found->second();
	return layout;
}

// Register a layout by passing a name and a function to create such a layout.
void MainFrameLayoutManager::registerLayout(
	const std::string& name, const CreateMainFrameLayoutFunc& func)
{
	std::pair<LayoutMap::iterator, bool> result = _layouts.insert(
		LayoutMap::value_type(name, func)
	);

	// Check if the insertion was successful
	if (!result.second) {
		rError() << "MainFrameLayoutManager: Layout "
			<< name << " already registered." << std::endl;
		return;
	}
}

void MainFrameLayoutManager::registerCommands() {
	// remove all commands beforehand
	_commands.clear();

	for (LayoutMap::const_iterator i = _layouts.begin(); i != _layouts.end(); ++i) {
		// add a new command for each layout
		_commands.push_back(
			LayoutCommandPtr(new LayoutCommand(i->first))
		);
	}
}

// RegisterableModule implementation
const std::string& MainFrameLayoutManager::getName() const {
	static std::string _name(MODULE_MAINFRAME_LAYOUT_MANAGER);
	return _name;
}

const StringSet& MainFrameLayoutManager::getDependencies() const {
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_UIMANAGER);
		_dependencies.insert(MODULE_COMMANDSYSTEM);
	}

	return _dependencies;
}

void MainFrameLayoutManager::initialiseModule(const ApplicationContext& ctx) {
	rMessage() << "MainFrameLayoutManager::initialiseModule called.\n";

	// Register the default layouts
	registerLayout(EMBEDDED_LAYOUT_NAME, EmbeddedLayout::CreateInstance);
	registerLayout(FLOATING_LAYOUT_NAME, FloatingLayout::CreateInstance);
	registerLayout(SPLITPANE_LAYOUT_NAME, SplitPaneLayout::CreateInstance);
	registerLayout(REGULAR_LAYOUT_NAME, RegularLayout::CreateRegularInstance);
	registerLayout(REGULAR_LEFT_LAYOUT_NAME, RegularLayout::CreateRegularLeftInstance);
}

void MainFrameLayoutManager::shutdownModule() {
	rMessage() << "MainFrameLayoutManager::shutdownModule called.\n";

	_commands.clear();
	_layouts.clear();
}

// Define the static MainFrameLayoutManager module
module::StaticModule<MainFrameLayoutManager> mainFrameLayoutManagerModule;

} // namespace ui
