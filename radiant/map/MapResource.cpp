#include "MapResource.h"

#include "i18n.h"
#include <sstream>
#include <fstream>
#include <iostream>
#include "ifiletypes.h"
#include "ientity.h"
#include "iarchive.h"
#include "igroupnode.h"
#include "ifilesystem.h"
#include "imainframe.h"
#include "iregistry.h"
#include "imapinfofile.h"

#include "map/Map.h"
#include "map/RootNode.h"
#include "mapfile.h"
#include "gamelib.h"
#include "wxutil/dialog/MessageBox.h"
#include "debugging/debugging.h"
#include "os/path.h"
#include "os/file.h"
#include "os/fs.h"
#include "map/algorithm/Traverse.h"
#include "stream/TextFileInputStream.h"
#include "scenelib.h"

#include <functional>
#include <fmt/format.h>

#include "infofile/InfoFile.h"
#include "string/string.h"

#include "algorithm/MapImporter.h"
#include "algorithm/MapExporter.h"
#include "infofile/InfoFileExporter.h"
#include "algorithm/ChildPrimitives.h"

namespace map
{

namespace
{
	const char* const GKEY_INFO_FILE_EXTENSION = "/mapFormat/infoFileExtension";

	// name may be absolute or relative
	inline std::string rootPath(const std::string& name) {
		return GlobalFileSystem().findRoot(
			path_is_absolute(name.c_str()) ? name : GlobalFileSystem().findFile(name)
		);
	}

	class NodeCounter :
		public scene::NodeVisitor
	{
	private:
		std::size_t _count;
	public:
		NodeCounter() :
			_count(0)
		{}

		bool pre(const scene::INodePtr& node)
		{
			if (Node_isPrimitive(node) || Node_isEntity(node))
			{
				_count++;
			}
			
			return true;
		}

		std::size_t getCount() const
		{
			return _count;
		}
	};
}

std::string MapResource::_infoFileExt;

// Constructor
MapResource::MapResource(const std::string& name) :
	_originalName(name),
	_type(os::getExtension(name))
{
	// Initialise the paths, this is all needed for realisation
    _path = rootPath(_originalName);
	_name = os::getRelativePath(_originalName, _path);

	if (_infoFileExt.empty())
	{
		_infoFileExt = game::current::getValue<std::string>(GKEY_INFO_FILE_EXTENSION);
	}

	if (!_infoFileExt.empty() && _infoFileExt[0] != '.') 
	{
		_infoFileExt = "." + _infoFileExt;
	}
}

void MapResource::rename(const std::string& fullPath)
{
	// Save the paths locally and split them into parts
	_originalName = fullPath;
	_type = fullPath.substr(fullPath.rfind(".") + 1);
	_path = rootPath(_originalName);
	_name = os::getRelativePath(_originalName, _path);

	// Rename the map root as well
    _mapRoot->setName(_name);
}

bool MapResource::load()
{
	if (!_mapRoot)
    {
		// Map not loaded yet, acquire map root node from loader
		_mapRoot = loadMapNode();

		connectMap();
		mapSave();
	}

	return _mapRoot != nullptr;
}

bool MapResource::save(const MapFormatPtr& mapFormat)
{
	// For saving, take the default map format for this game type
	MapFormatPtr format = mapFormat ? mapFormat : GlobalMapFormatManager().getMapFormatForGameType(
		GlobalGameManager().currentGame()->getKeyValue("type"), _type
	);

	if (format == NULL)
	{
		rError() << "Could not locate map format module." << std::endl;
		return false;
	}

	rMessage() << "Using " << format->getMapFormatName() << " format to save the resource." << std::endl;
	
	std::string fullpath = _path + _name;

	// Save a backup of the existing file (rename it to .bak) if it exists in the first place
	if (os::fileOrDirExists(fullpath))
	{
		if (!saveBackup())
		{
			// angua: if backup creation is not possible, still save the map
			// but create message in the console
			rError() << "Could not create backup (Map is possibly open in Doom3)" << std::endl;
			// return false;
		}
	}

	bool success = false;

	if (path_is_absolute(fullpath.c_str()))
	{
		// Save the actual file
		success = saveFile(*format, _mapRoot, map::traverse, fullpath);
	}
	else
	{
		rError() << "Map path is not absolute: " << fullpath << std::endl;
		success = false;
	}

	if (success)
	{
  		mapSave();
  		return true;
	}

	return false;
}

bool MapResource::saveBackup()
{
	fs::path fullpath = (_path + _name);

	if (path_is_absolute(fullpath.string().c_str()))
	{
		// Save a backup if possible. This is done by renaming the original,
		// which won't work if the existing map is currently open by Doom 3
		// in the background.
		if (!fs::exists(fullpath))
		{
			return false;
		}

		fs::path auxFile = fullpath;
		auxFile.replace_extension(_infoFileExt);

		if (os::fileIsWritable(fullpath.string()))
		{
			fs::path backup = fullpath;
			backup.replace_extension(".bak");
			
			// replace_extension() doesn't accept something like ".darkradiant.bak", so roll our own
			fs::path auxFileBackup = auxFile.string() + ".bak";

			bool errorOccurred = false;

			try
			{
				// remove backup
				if (fs::exists(backup))
				{
					fs::remove(backup);
				}

				// rename current to backup
				fs::rename(fullpath, backup);
			}
			catch (fs::filesystem_error& ex)
			{
				rWarning() << "Error while creating backups: " << ex.what() << 
					", the file is possibly opened by the game." << std::endl;
				errorOccurred = true;
			}

			try
			{
				// remove aux file backup
				if (fs::exists(auxFileBackup))
				{
					fs::remove(auxFileBackup);
				}

				// Check if the .darkradiant file exists in the first place
				if (fs::exists(auxFile))
				{
					// rename current to backup
					fs::rename(auxFile, auxFileBackup);
				}
			}
			catch (fs::filesystem_error& ex)
			{
				rWarning() << "Error while creating backups: " << ex.what() << 
					", the file is possibly opened by the game." << std::endl;
				errorOccurred = true;
			}

			return !errorOccurred;
		}
		else
		{
			rError() << "map path is not writeable: " << fullpath.string() << std::endl;

			// File is write-protected
			wxutil::Messagebox::ShowError(
				fmt::format(_("File is write-protected: {0}"), fullpath.string()));

			return false;
		}
	}

	return false;
}

scene::IMapRootNodePtr MapResource::getNode()
{
	return _mapRoot;
}

void MapResource::setNode(const scene::IMapRootNodePtr& node)
{
    _mapRoot = std::dynamic_pointer_cast<RootNode>(node);
	connectMap();
}

void MapResource::onMapChanged() 
{
	GlobalMap().setModified(true);
}

void MapResource::connectMap()
{
    if (_mapRoot)
    {
        // Reroute the changed callback to the onMapChanged() call.
        _mapRoot->getUndoChangeTracker().setChangedCallback(std::bind(&MapResource::onMapChanged, this));
    }
}

void MapResource::mapSave()
{
    if (_mapRoot)
    {
        _mapRoot->getUndoChangeTracker().save();
    }
}

MapFormatPtr MapResource::determineMapFormat(std::istream& stream)
{
	// Get all registered map formats
	std::set<MapFormatPtr> availableFormats = GlobalMapFormatManager().getMapFormatList(_type);

	MapFormatPtr format;

	for (const MapFormatPtr& candidate : availableFormats)
	{
		// Rewind the stream before passing it to the format for testing
		// Map format valid, rewind the stream
		stream.seekg(0, std::ios_base::beg);

		if (candidate->canLoad(stream))
		{
			format = candidate;
			break;
		}
	}

	// Rewind the stream when we're done
	stream.seekg(0, std::ios_base::beg);

	return format;
}

RootNodePtr MapResource::loadMapNode()
{
	RootNodePtr rootNode;

	// greebo: Check if we have valid settings
	// The _path might be empty if we're loading from a folder outside the mod
	if (_name.empty() && _type.empty())
	{
        return rootNode;
	}

	try
	{
		// Build the map path
		std::string fullpath = _path + _name;

		// Open a stream (from physical file or VFS)
		openFileStream(fullpath, [&](std::istream& mapStream)
		{
			rootNode = loadMapNodeFromStream(mapStream, fullpath);
		});
	}
	catch (std::runtime_error& ex)
	{
		wxutil::Messagebox::ShowError(ex.what());
	}

	return rootNode;
}

RootNodePtr MapResource::loadMapNodeFromStream(std::istream& stream, const std::string& fullpath)
{
	// Get the mapformat
	MapFormatPtr format = determineMapFormat(stream);

	if (format == NULL)
	{
		throw std::runtime_error(
			fmt::format(_("Could not determine map format of file:\n{0}"), fullpath));
	}

	// Map format valid, rewind the stream
	stream.seekg(0, std::ios_base::beg);
	
	// Create a new map root node
    RootNodePtr root = std::make_shared<RootNode>(_name);

	if (loadFile(stream, *format, root, fullpath))
	{
		return root;
	}

    return RootNodePtr();
}

bool MapResource::loadFile(std::istream& mapStream, const MapFormat& format, const RootNodePtr& root, const std::string& filename)
{
	// Our importer taking care of scene insertion
	MapImporter importFilter(root, mapStream);

	// Acquire a map reader/parser
	IMapReaderPtr reader = format.getMapReader(importFilter);

	try
	{
		// Start parsing
		reader->readFromStream(mapStream);

		// Prepare child primitives
		addOriginToChildPrimitives(root);

		if (!format.allowInfoFileCreation())
		{
			// No info file handling, just return success
			return true;
		}

		// Check for an additional info file
		loadInfoFile(root, filename, importFilter.getNodeMap());

		return true;
	}
	catch (wxutil::ModalProgressDialog::OperationAbortedException&)
	{
		wxutil::Messagebox::ShowError(
			_("Map loading cancelled")
		);

		// Clear out the root node, otherwise we end up with half a map
		scene::NodeRemover remover;
		root->traverseChildren(remover);

		return false;
	}
	catch (IMapReader::FailureException& e)
	{
		wxutil::Messagebox::ShowError(
				fmt::format(_("Failure reading map file:\n{0}\n\n{1}"), filename, e.what()));

		// Clear out the root node, otherwise we end up with half a map
		scene::NodeRemover remover;
		root->traverseChildren(remover);

		return false;
	}
}

void MapResource::loadInfoFile(const RootNodePtr& root, const std::string& filename, const NodeIndexMap& nodeMap)
{
	try
	{
		std::string infoFilename(filename.substr(0, filename.rfind('.')));
		infoFilename += game::current::getValue<std::string>(GKEY_INFO_FILE_EXTENSION);

		openFileStream(infoFilename, [&](std::istream& infoFileStream)
		{
			loadInfoFileFromStream(infoFileStream, root, nodeMap);
		});
	}
	catch (std::runtime_error& ex)
	{
		rWarning() << ex.what() << std::endl;
	}
}

void MapResource::loadInfoFileFromStream(std::istream& infoFileStream, const RootNodePtr& root, const NodeIndexMap& nodeMap)
{
	if (!infoFileStream.good())
	{
		rError() << "[MapResource] No valid info file stream" << std::endl;
		return;
	}

	rMessage() << "Parsing info file..." << std::endl;

	try
	{
		// Read the infofile
		InfoFile infoFile(infoFileStream, root, nodeMap);

		// Start parsing, this will throw if any errors occur
		infoFile.parse();
	}
	catch (parser::ParseException& e)
	{
		rError() << "[MapResource] Unable to parse info file: " << e.what() << std::endl;
	}
}

void MapResource::openFileStream(const std::string& path, const std::function<void(std::istream&)>& streamProcessor)
{
	if (path_is_absolute(path.c_str()))
	{
		rMessage() << "Open file " << path << " from filesystem...";

		TextFileInputStream file(path);

		if (file.failed())
		{
			rError() << "failure" << std::endl;
			throw std::runtime_error(fmt::format(_("Failure opening file:\n{0}"), path));
		}

		std::istream stream(&file);

		rMessage() << "success." << std::endl;

		streamProcessor(stream);
	}
	else
	{
		// Not an absolute path, might as well be a VFS path, so try to load it from the PAKs
		rMessage() << "Trying to open file " << path << " from VFS...";

		ArchiveTextFilePtr vfsFile = GlobalFileSystem().openTextFile(path);

		if (!vfsFile)
		{
			rError() << "failure" << std::endl;
			throw std::runtime_error(fmt::format(_("Failure opening file:\n{0}"), path));
		}

		rMessage() << "success." << std::endl;

		std::istream vfsStream(&(vfsFile->getInputStream()));

		// Deflated text files don't support stream positioning (seeking)
		// so load everything into one large string and create a new buffer
		std::stringstream stringStream;
		stringStream << vfsStream.rdbuf();

		streamProcessor(stringStream);
	}
}

bool MapResource::checkIsWriteable(const fs::path& path)
{
	// Check writeability of the given file
	if (os::fileOrDirExists(path.string()) && !os::fileIsWritable(path))
	{
		// File is write-protected
		rError() << "File is write-protected." << std::endl;

		wxutil::Messagebox::ShowError(
			fmt::format(_("File is write-protected: {0}"), path.string()));

		return false;
	}

	return true;
}

bool MapResource::saveFile(const MapFormat& format, const scene::INodePtr& root,
						   const GraphTraversalFunc& traverse, const std::string& filename)
{
	// Actual output file paths
	fs::path outFile = filename;
	fs::path auxFile = outFile;
	auxFile.replace_extension(_infoFileExt);

	// Check writeability of the primary output file
	if (!checkIsWriteable(outFile)) return false;

	// Test opening the output file
	rMessage() << "Opening file " << outFile.string();

	// Open the stream to the primary output file
	std::unique_ptr<std::ofstream> outFileStream(new std::ofstream(outFile.string()));
	std::unique_ptr<std::ofstream> auxFileStream; // aux stream is optional

	// Check writeability of the auxiliary output file if necessary
	if (format.allowInfoFileCreation())
	{
		if (!checkIsWriteable(auxFile)) return false;

		rMessage() << " and auxiliary file " << auxFile.string();

		auxFileStream.reset(new std::ofstream(auxFile.string()));
	}

	rMessage() << " for writing... ";

	if (outFileStream->is_open() && (!auxFileStream || auxFileStream->is_open()))
	{
		rMessage() << "success" << std::endl;

		// Check the total count of nodes to traverse
		NodeCounter counter;
		traverse(root, counter);
		
		// Acquire the MapWriter from the MapFormat class
		IMapWriterPtr mapWriter = format.getMapWriter();

		// Create our main MapExporter walker, and pass the desired 
		// writer to it. The constructor will prepare the scene
		// and the destructor will clean it up afterwards. That way
		// we ensure a nice and tidy scene when exceptions are thrown.
		MapExporterPtr exporter;
		
		if (format.allowInfoFileCreation())
		{
			exporter.reset(new MapExporter(*mapWriter, root, *outFileStream, *auxFileStream, counter.getCount()));
		}
		else
		{
			exporter.reset(new MapExporter(*mapWriter, root, *outFileStream, counter.getCount())); // no aux stream
		}

		bool cancelled = false;

		try
		{
			// Pass the traversal function and the root of the subgraph to export
			exporter->exportMap(root, traverse);
		}
		catch (wxutil::ModalProgressDialog::OperationAbortedException&)
		{
			wxutil::Messagebox::ShowError(_("Map writing cancelled"));

			cancelled = true;
		}

		exporter.reset();

		outFileStream->close();

		if (auxFileStream)
		{
			auxFileStream->close();
		}

		return !cancelled;
	}
	else
	{
		wxutil::Messagebox::ShowError(_("Could not open output streams for writing"));

		rError() << "failure" << std::endl;
		return false;
	}
}

} // namespace map
