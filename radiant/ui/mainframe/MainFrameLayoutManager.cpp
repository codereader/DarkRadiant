#include "MainFrameLayoutManager.h"

#include "itextstream.h"
#include "ui/igroupdialog.h"
#include "icommandsystem.h"

#include "module/StaticModule.h"

#include "AuiLayout.h"

namespace ui
{

IMainFrameLayoutPtr MainFrameLayoutManager::getLayout(const std::string& name)
{
	// Try to lookup that layout
	auto found = _layouts.find(name);

	if (found == _layouts.end())
    {
		rError() << "MainFrameLayoutManager: Could not find " << name << std::endl;
		return IMainFrameLayoutPtr();
	}

	// Call the creation function and return the layout
	return found->second();
}

void MainFrameLayoutManager::registerLayout(
	const std::string& name, const CreateMainFrameLayoutFunc& func)
{
    // Check if the insertion was successful
    if (!_layouts.emplace(name, func).second)
    {
        rError() << "MainFrameLayoutManager: Layout " << name << " already registered." << std::endl;
    }
}

void MainFrameLayoutManager::registerCommands()
{
    // remove all commands beforehand
    _commands.clear();

    // Don't bother adding the menu if we just have one layout
    if (_layouts.size() > 1)
    {
        // Add a new command for each layout
        for (auto i = _layouts.begin(); i != _layouts.end(); ++i)
        {
            _commands.emplace_back(std::make_shared<LayoutCommand>(i->first));
        }
    }
}

// RegisterableModule implementation
const std::string& MainFrameLayoutManager::getName() const
{
	static std::string _name(MODULE_MAINFRAME_LAYOUT_MANAGER);
	return _name;
}

const StringSet& MainFrameLayoutManager::getDependencies() const
{
    static StringSet _dependencies { MODULE_COMMANDSYSTEM };
	return _dependencies;
}

void MainFrameLayoutManager::initialiseModule(const IApplicationContext& ctx)
{
	// Register the default layout
	registerLayout(AUI_LAYOUT_NAME, AuiLayout::CreateInstance);
}

void MainFrameLayoutManager::shutdownModule()
{
	_commands.clear();
	_layouts.clear();
}

// Define the static MainFrameLayoutManager module
module::StaticModuleRegistration<MainFrameLayoutManager> mainFrameLayoutManagerModule;

} // namespace ui
