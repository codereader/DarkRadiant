#include "Doom3MapCompiler.h"

#include "itextstream.h"
#include "icommandsystem.h"
#include "ientity.h"
#include "iregistry.h"
#include "ifilesystem.h"

#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "os/path.h"
#include "os/file.h"
#include "stream/textfilestream.h"
#include "scene/Node.h"
#include "../Doom3MapReader.h"

namespace map
{

	namespace
	{
		class BasicNode :
			public scene::Node
		{
		private:
			AABB _emptyAABB;

		public:
			// Renderable implementation (empty)
			void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
			{}

			void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
			{}

			const AABB& localAABB() const
			{
				return _emptyAABB;
			}

			bool isHighlighted() const
			{
				return false; // never highlighted
			}
		};

		class RawImporter :
			public IMapImportFilter
		{
		private:
			scene::INodePtr _root;

		public:
			RawImporter(const scene::INodePtr& root) :
				_root(root)
			{}

			bool addEntity(const scene::INodePtr& entityNode)
			{
				_root->addChildNode(entityNode);
				return true;
			}

			bool addPrimitiveToEntity(const scene::INodePtr& primitive, const scene::INodePtr& entity)
			{
				if (Node_getEntity(entity)->isContainer())
				{
					entity->addChildNode(primitive);
					return true;
				}
				else
				{
					return false;
				}
			}
		};
	}

void Doom3MapCompiler::generateProc(const scene::INodePtr& root)
{
	globalOutputStream() << "=== DMAP: GenerateProc ===" << std::endl;

	
}

void Doom3MapCompiler::runDmap(const scene::INodePtr& root)
{
	assert(root);

	// Load the data from the .map file into our working format

}

void Doom3MapCompiler::runDmap(const std::string& mapFile)
{
	if (!os::fileOrDirExists(mapFile) || file_is_directory(mapFile.c_str()))
	{
		globalErrorStream() << "Can't dmap, file doesn't exist: " << mapFile << std::endl;
		return;
	}

	TextFileInputStream file(mapFile);
	std::istream mapStream(&file);

	boost::shared_ptr<BasicNode> root(new BasicNode);

	RawImporter importFilter(root);

	try
	{
		// Parse our map file
		Doom3MapReader reader(importFilter);
		reader.readFromStream(mapStream);
	}
	catch (IMapReader::FailureException& e)
	{
		globalErrorStream() << 
			(boost::format("Failure reading map file:\n%s\n\n%s") % mapFile % e.what()).str() << std::endl;
		return;
	}

	// Start the sequence
	runDmap(root);
}

void Doom3MapCompiler::dmapCmd(const cmd::ArgumentList& args)
{
	if (args.size() != 1)
	{
		globalWarningStream() << "Usage: dmap <mapFile>" << std::endl;
		return;
	}

	std::string mapFile = args[0].getString();
	
	if (!boost::algorithm::iends_with(mapFile, ".map"))
	{
		mapFile.append(".map");
	}

	std::string mapPath = mapFile;

	// Find the map file
	if (!path_is_absolute(mapPath.c_str()))
	{
		mapPath = GlobalFileSystem().findFile(mapFile);

		if (mapPath.empty())
		{
			// Try again with maps/ prepended
			mapFile = "maps/" + mapFile;
			mapPath = GlobalFileSystem().findFile(mapFile);
		}

		mapPath += mapFile;
	}

	// Start the sequence
	runDmap(mapPath);
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
