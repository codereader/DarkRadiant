#pragma once

#include "imodule.h"
#include "iorthocontextmenu.h"

namespace ui
{

/**
 * Module responsible of registering and intialising the various
 * UI classes in DarkRadiant, e.g. the LayerSystem.
 * 
 * Currently many UI classes are spread and initialised all across
 * the main binary, so there's still work left to do.
 */
class UserInterfaceModule :
	public RegisterableModule
{
public:
	// RegisterableModule
	const std::string & getName() const override;
	const StringSet & getDependencies() const override;
	void initialiseModule(const ApplicationContext & ctx) override;
	void shutdownModule() override;

private:
	void registerUICommands();
};

}
