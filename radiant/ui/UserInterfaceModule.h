#pragma once

#include "imodule.h"
#include "iorthocontextmenu.h"
#include "icommandsystem.h"

#include "EntityClassColourManager.h"
#include "LongRunningOperationHandler.h"

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
private:
	std::unique_ptr<EntityClassColourManager> _eClassColourManager;
	std::unique_ptr<LongRunningOperationHandler> _longOperationHandler;

public:
	// RegisterableModule
	const std::string & getName() const override;
	const StringSet & getDependencies() const override;
	void initialiseModule(const ApplicationContext & ctx) override;
	void shutdownModule() override;

private:
	void registerUICommands();
	void refreshShadersCmd(const cmd::ArgumentList& args);
};

}
