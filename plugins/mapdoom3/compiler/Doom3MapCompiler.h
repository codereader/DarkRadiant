#pragma once

#include "imapcompiler.h"
#include "icommandsystem.h"
#include "inode.h"

#include "ProcFile.h"
#include "DebugRenderer.h"

namespace map
{

class Doom3MapCompiler :
	public IMapCompiler
{
private:
	DebugRendererPtr _debugRenderer;
	ProcFilePtr _procFile;

public:
	virtual void generateProc(const scene::INodePtr& root);

	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
	virtual void shutdownModule();

private:
	//  The method called by the "dmap" command
	void dmapCmd(const cmd::ArgumentList& args);

	void setDmapRenderOption(const cmd::ArgumentList& args);

	// Runs the actual dmap sequence on the given map file
	void runDmap(const scene::INodePtr& root);
	void runDmap(const std::string& mapFile);
};
typedef boost::shared_ptr<Doom3MapCompiler> Doom3MapCompilerPtr;

} // namespace
