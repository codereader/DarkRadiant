#include "Doom3MapCompiler.h"

#include "itextstream.h"
#include "icommandsystem.h"
#include "iregistry.h"
#include "ifilesystem.h"
#include <boost/bind.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "os/path.h"
#include "os/file.h"

namespace map
{

void Doom3MapCompiler::generateProc(const std::string& mapFile)
{
	globalOutputStream() << "=== DMAP: GenerateProc ===" << std::endl;


}

void Doom3MapCompiler::dmapCmd(const cmd::ArgumentList& args)
{
	if (args.size() != 1)
	{
		globalWarningStream() << "Usage: dmap <mapFile>" << std::endl;
		return;
	}

	std::string mapPath = args[0].getString();

	// Find the map file
	if (!path_is_absolute(mapPath.c_str()))
	{
		if (!boost::algorithm::iends_with(mapPath, ".map"))
		{
			mapPath.append(".map");
		}

		mapPath = GlobalFileSystem().findFile(mapPath);

		if (mapPath.empty())
		{
			// Try again with maps/ prepended
			mapPath = GlobalFileSystem().findFile("maps/" + mapPath);
		}
	}

	if (!os::fileOrDirExists(mapPath) || file_is_directory(mapPath.c_str()))
	{
		globalErrorStream() << "Can't dmap, file doesn't exist: " << mapPath << std::endl;
		return;
	}

	// Start the sequence
	generateProc(mapPath);
}

// RegisterableModule implementation
const std::string& Doom3MapCompiler::getName() const
{
	static std::string _name(MODULE_MAPCOMPILER);
	return _name;
}

const StringSet& Doom3MapCompiler::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_COMMANDSYSTEM);
	}

	return _dependencies;
}

void Doom3MapCompiler::initialiseModule(const ApplicationContext& ctx)
{
	globalOutputStream() << getName() << ": initialiseModule called." << std::endl;

	GlobalCommandSystem().addCommand("dmap", boost::bind(&Doom3MapCompiler::dmapCmd, this, _1), cmd::ARGTYPE_STRING);
}

void Doom3MapCompiler::shutdownModule()
{
}

} // namespace
