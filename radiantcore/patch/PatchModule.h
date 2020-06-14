#pragma once

#include <sigc++/connection.h>
#include "ipatch.h"
#include "PatchSettings.h"

namespace patch
{

class PatchModule :
	public IPatchModule
{
private:
	std::unique_ptr<PatchSettings> _settings;

	sigc::connection _patchTextureChanged;

public:
	// PatchCreator implementation
	scene::INodePtr createPatch(PatchDefType type) override;

	IPatchSettings& getSettings() override;

	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const ApplicationContext& ctx) override;
	void shutdownModule() override;

private:
	void registerPatchCommands();
};

}
