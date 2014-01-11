#pragma once 

#include "imodule.h"
#include "inode.h"

namespace map
{

/**
 * Map Compiler interface for creating D3-compatible .proc files.
 */
class IMapCompiler :
	public RegisterableModule
{
public:
    virtual ~IMapCompiler() {}

	// Use the given map file to generate the .proc file containing the pre-processed map models
	virtual void generateProc(const scene::INodePtr& root) = 0;
};
typedef boost::shared_ptr<IMapCompiler> IMapCompilerPtr;

}

const char* const MODULE_MAPCOMPILER = "MapCompiler";

// Application-wide Accessor to the global map compiler
inline map::IMapCompiler& GlobalMapCompiler()
{
	// Cache the reference locally
	static map::IMapCompiler& _mapCompiler(
		*boost::static_pointer_cast<map::IMapCompiler>(
			module::GlobalModuleRegistry().getModule(MODULE_MAPCOMPILER)
		)
	);
	return _mapCompiler;
}
