#include "MainFrameLayoutManager.h"

#include "itextstream.h"
#include "ieventmanager.h"
#include "imainframe.h"

#include "generic/callback.h"
#include "modulesystem/StaticModule.h"

#include "FloatingLayout.h"

namespace ui {

IMainFrameLayoutPtr MainFrameLayoutManager::getLayout(const std::string& name) {
	// Try to lookup that layout
	LayoutMap::const_iterator found = _layouts.find(name);
	
	if (found == _layouts.end()) {
		globalErrorStream() << "MainFrameLayoutManager: Could not find " << name << std::endl;
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
		globalErrorStream() << "MainFrameLayoutManager: Layout " 
			<< name << " already registered." << std::endl;
		return;
	}
}

void MainFrameLayoutManager::toggleFloating() {
	// Check if active
	if (GlobalMainFrame().getCurrentLayout() == FLOATING_LAYOUT_NAME) {
		// Remove the active floating layout
		GlobalMainFrame().applyLayout("");
	}
	else {
		GlobalMainFrame().applyLayout(FLOATING_LAYOUT_NAME);
	}
}

// RegisterableModule implementation
const std::string& MainFrameLayoutManager::getName() const {
	static std::string _name(MODULE_MAINFRAME_LAYOUT_MANAGER);
	return _name;
}

const StringSet& MainFrameLayoutManager::getDependencies() const {
	static StringSet _dependencies;
	return _dependencies;
}

void MainFrameLayoutManager::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "MainFrameLayoutManager::initialiseModule called.\n";

	// Register the default layouts
	registerLayout(FLOATING_LAYOUT_NAME, FloatingLayout::CreateInstance);

	GlobalEventManager().addCommand("ToggleLayoutFloating",
		MemberCaller<MainFrameLayoutManager, &MainFrameLayoutManager::toggleFloating>(*this)
	);
}

void MainFrameLayoutManager::shutdownModule() {
	globalOutputStream() << "MainFrameLayoutManager::shutdownModule called.\n";

	_layouts.clear();
}

// Define the static MainFrameLayoutManager module
module::StaticModule<MainFrameLayoutManager> mainFrameLayoutManagerModule;

} // namespace ui
