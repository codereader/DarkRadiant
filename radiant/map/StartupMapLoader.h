#pragma once

#include "iradiant.h"
#include "imodule.h"
#include <memory>

namespace map
{

class StartupMapLoader :
	public RegisterableModule
{
public:
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const ApplicationContext& ctx) override;

private:
	// This gets called as soon as the mainframe starts up
	void onRadiantStartup();

	// Called when the mainframe shuts down
	void onRadiantShutdown();

	void loadMapSafe(const std::string& map);
};

} // namespace map
