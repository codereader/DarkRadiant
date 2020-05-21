#pragma once

#include "ipatch.h"
#include "PatchSettings.h"

namespace patch
{

class PatchModule :
	public IPatchModule
{
private:
	std::unique_ptr<PatchSettings> _settings;

public:
	// PatchCreator implementation
	scene::INodePtr createPatch(PatchDefType type) override;

	IPatchSettings& getSettings() override;

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);

private:
	void registerPatchCommands();
};

}
