#pragma once

#include "ui/imainframelayout.h"
#include <map>
#include <vector>
#include <string>

#include "LayoutCommand.h"

namespace ui
{

class MainFrameLayoutManager :
	public IMainFrameLayoutManager
{
	// A map associating names with creation functions
    std::map<std::string, CreateMainFrameLayoutFunc> _layouts;

	// The container for the LayoutCommand objects.
	std::vector<LayoutCommandPtr> _commands;

public:
	// Retrieves a layout with the given name. Returns NULL if not found.
	IMainFrameLayoutPtr getLayout(const std::string& name) override;

	// Register a layout by passing a name and a function to create such a layout.
	void registerLayout(const std::string& name, const CreateMainFrameLayoutFunc& func) override;

	// Adds all commands for all layouts registered so far
	void registerCommands() override;

	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;
	void shutdownModule() override;
};

} // namespace ui
