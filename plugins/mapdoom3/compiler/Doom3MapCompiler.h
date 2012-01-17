#pragma once

#include "imapcompiler.h"
#include "icommandsystem.h"

namespace map
{

class Doom3MapCompiler :
	public IMapCompiler
{
public:
	virtual void generateProc(const std::string& mapFile);

	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
	virtual void shutdownModule();

private:
	//  The method called by the "dmap" command
	void dmapCmd(const cmd::ArgumentList& args);
};
typedef boost::shared_ptr<Doom3MapCompiler> Doom3MapCompilerPtr;

} // namespace
