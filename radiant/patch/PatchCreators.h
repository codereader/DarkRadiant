#pragma once

#include "ipatch.h"
#include "PatchSettings.h"

namespace patch
{

/**
 * greebo: The Doom3PatchCreator implements the method createPatch(),
 * as required by the abstract base class PatchCreator (see ipatch.h).
 */
class Doom3PatchCreator :
	public PatchCreator
{
private:
	std::unique_ptr<PatchSettings> _settings;

public:
	// PatchCreator implementation
	scene::INodePtr createPatch() override;

	IPatchSettings& getSettings() override;

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);

private:
	void registerPatchCommands();
};

/* greebo: This is the same as the above, but makes sure that a patchDef2 node is created.
 */
class Doom3PatchDef2Creator :
	public PatchCreator
{
public:
	// PatchCreator implementation
	scene::INodePtr createPatch();

	IPatchSettings& getSettings() override
	{
		throw std::runtime_error("Not applicable for this type, use DEF3");
	}

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
};

}
