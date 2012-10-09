#pragma once

#include "ipatch.h"

/**
 * greebo: The Doom3PatchCreator implements the method createPatch(),
 * as required by the abstract base class PatchCreator (see ipatch.h).
 */
class Doom3PatchCreator :
	public PatchCreator
{
public:
	// PatchCreator implementation
	scene::INodePtr createPatch();

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

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
};
