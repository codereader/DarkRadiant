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
#include "iregistry.h"
#include "imapinfofile.h"

#include "map/Map.h"
#include "map/RootNode.h"
#include "mapfile.h"
#include "gamelib.h"
#include "debugging/debugging.h"
#include "os/path.h"
#include "os/file.h"
#include "os/fs.h"
#include "scene/Traverse.h"
#include "scenelib.h"

#include <functional>
#include <fmt/format.h>

#include "infofile/InfoFile.h"
#include "string/string.h"

#include "algorithm/MapExporter.h"
#include "algorithm/Import.h"
#include "infofile/InfoFileExporter.h"
#include "messages/MapFileOperation.h"
#include "NodeCounter.h"
#include "MapResourceLoader.h"

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
}

MapResource::MapResource(const std::string& resourcePath)
{
    constructPaths(resourcePath);
}

void MapResource::rename(const std::string& fullPath)
{
    constructPaths(fullPath);

	// Rename the map root as well
    _mapRoot->setName(_name);
}

void MapResource::constructPaths(const std::string& resourcePath)
{
    // Since the resource path can contain dots like this ".." 
    // pass the filename part only to getExtension().
    _extension = os::getExtension(os::getFilename(resourcePath));

    // Try to find a folder part of the VFS and use that as base path
    // Will result to an empty string if the path is outside the VFS
    _path = rootPath(resourcePath);

    // Try to create a relative path, based on the VFS directories
    // If no relative path can be deducted, use the absolute resourcePath 
    // in unmodified form.
    _name = os::getRelativePath(resourcePath, _path);
}

std::string MapResource::getAbsoluteResourcePath()
{
    // Concatenate path+name, since they either contain base + relative:
    //      _path == "c:/games/darkmod/"
    //      _name == "maps/arkham.map"
    // or an empty _path with _name holding the full path:
    //      _path == ""
    //      _name == "c:/some/non/vfs/folder/arkham.map"
    return _path + _name;
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

void MapResource::save(const MapFormatPtr& mapFormat)
{
	// For saving, take the default map format for this game type
	MapFormatPtr format = mapFormat ? mapFormat : GlobalMapFormatManager().getMapFormatForGameType(
		GlobalGameManager().currentGame()->getKeyValue("type"), _extension
	);

	if (!format)
	{
		rError() << "Could not locate map format module." << std::endl;
		throw OperationException(_("Failed to locate map format module"));
	}

	rMessage() << "Using " << format->getMapFormatName() << " format to save the resource." << std::endl;
	
	std::string fullpath = getAbsoluteResourcePath();

	// Save a backup of the existing file (rename it to .bak) if it exists in the first place
	if (os::fileOrDirExists(fullpath) && !saveBackup())
	{
		// angua: if backup creation is not possible, still save the map
		// but create message in the console
		rError() << "Could not create backup (Map is possibly open in Doom3)" << std::endl;
	}

	if (!path_is_absolute(fullpath.c_str()))
	{
		rError() << "Map path is not absolute: " << fullpath << std::endl;
		throw OperationException(fmt::format(_("Map path is not absolute: {0}"), fullpath));
	}

	// Save the actual file (throws on fail)
	saveFile(*format, _mapRoot, scene::traverse, fullpath);

	mapSave();
}

bool MapResource::saveBackup()
{
	fs::path fullpath = getAbsoluteResourcePath();

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
		auxFile.replace_extension(getInfoFileExtension());

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

		// Handle the .darkradiant file only if the above succeeded
		if (!errorOccurred)
		{
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
			catch (fs::filesystem_error & ex)
			{
				rWarning() << "Error while creating backups: " << ex.what() <<
					", the file is possibly opened by the game." << std::endl;
				errorOccurred = true;
			}
		}

		return !errorOccurred;
	}

	return false;
}

const scene::IMapRootNodePtr& MapResource::getRootNode()
{
	return _mapRoot;
}

void MapResource::clear()
{
    _mapRoot = std::make_shared<RootNode>("");
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

RootNodePtr MapResource::loadMapNode()
{
	RootNodePtr rootNode;

	// Build the map path
	auto fullpath = getAbsoluteResourcePath();

	// Open a stream (from physical file or VFS) - will throw on failure
    auto stream = openFileStream(fullpath);

    try
    {
        // Get the mapformat
        auto format = algorithm::determineMapFormat(stream->getStream(), _extension);

        if (!format)
        {
            throw OperationException(_("Could not determine map format"));
        }

        // Instantiate a loader to process the map file stream
        MapResourceLoader loader(stream->getStream(), *format);

        // Load the root from the primary stream (throws on failure or cancel)
        rootNode = loader.load();

        if (rootNode)
        {
            rootNode->setName(_name);
        }

        // Check if an info file is supported by this map format
        if (format->allowInfoFileCreation())
        {
            try
            {
                // Load for an additional info file
                auto infoFilename = fullpath.substr(0, fullpath.rfind('.'));
                infoFilename += getInfoFileExtension();

                auto infoFileStream = openFileStream(infoFilename);

                if (infoFileStream->isOpen())
                {
                    loader.loadInfoFile(infoFileStream->getStream(), rootNode);
                }
            }
            catch (const OperationException& ex)
            {
                // Info file load file does not stop us, just issue a warning
                rWarning() << ex.what() << std::endl;
            }
        }
    }
    catch (const OperationException& ex)
    {
        // Re-throw the exception, prepending the map file path to the message (if not cancelled)
        throw ex.operationCancelled() ? ex : 
            OperationException(fmt::format(_("Failure reading map file:\n{0}\n\n{1}"), fullpath, ex.what()));
    }

	return rootNode;
}

stream::MapResourceStream::Ptr MapResource::openFileStream(const std::string& path)
{
    // Call the factory method to acquire a stream
    auto stream = stream::MapResourceStream::OpenFromPath(path);

    if (!stream->isOpen())
    {
        throw OperationException(fmt::format(_("Could not open file:\n{0}"), path));
    }

    return stream;
}

std::string MapResource::getInfoFileExtension()
{
    std::string extension = game::current::getValue<std::string>(GKEY_INFO_FILE_EXTENSION);

    if (!extension.empty() && extension[0] != '.')
    {
        extension = "." + extension;
    }

    return extension;
}

void MapResource::throwIfNotWriteable(const fs::path& path)
{
	// Check writeability of the given file
	if (os::fileOrDirExists(path.string()) && !os::fileIsWritable(path))
	{
		// File is write-protected
		rError() << "File is write-protected." << std::endl;

		throw OperationException(fmt::format(_("File is write-protected: {0}"), path.string()));
	}
}

void MapResource::saveFile(const MapFormat& format, const scene::IMapRootNodePtr& root,
						   const GraphTraversalFunc& traverse, const std::string& filename)
{
	// Actual output file paths
	fs::path outFile = filename;
	fs::path auxFile = outFile;
	auxFile.replace_extension(getInfoFileExtension());

	// Check writeability of the primary output file
	throwIfNotWriteable(outFile);

	// Test opening the output file
	rMessage() << "Opening file " << outFile.string();

	// Open the stream to the primary output file
	std::ofstream outFileStream(outFile.string());
	std::unique_ptr<std::ofstream> auxFileStream; // aux stream is optional

	// Check writeability of the auxiliary output file if necessary
	if (format.allowInfoFileCreation())
	{
		rMessage() << " and auxiliary file " << auxFile.string();

		throwIfNotWriteable(auxFile);

		auxFileStream.reset(new std::ofstream(auxFile.string()));
	}

	rMessage() << " for writing... ";

	if (!outFileStream.is_open())
	{
		throw OperationException(fmt::format(_("Could not open file for writing: {0}"), outFile.string()));
	}

	if (auxFileStream && !auxFileStream->is_open())
	{
		throw OperationException(fmt::format(_("Could not open file for writing: {0}"), auxFile.string()));
	}

	rMessage() << "success" << std::endl;

	// Check the total count of nodes to traverse
	NodeCounter counter;
	traverse(root, counter);
		
	// Create our main MapExporter walker, and pass the desired 
	// format to it. The constructor will prepare the scene
	// and the destructor will clean it up afterwards. That way
	// we ensure a nice and tidy scene when exceptions are thrown.
	MapExporterPtr exporter;
	auto mapWriter = format.getMapWriter();

	if (format.allowInfoFileCreation())
	{
		exporter.reset(new MapExporter(*mapWriter, root, outFileStream, *auxFileStream, counter.getCount()));
	}
	else
	{
		exporter.reset(new MapExporter(*mapWriter, root, outFileStream, counter.getCount())); // no aux stream
	}

	try
	{
		// Pass the traversal function and the root of the subgraph to export
		exporter->exportMap(root, traverse);
	}
	catch (FileOperation::OperationCancelled&)
	{
		throw OperationException(_("Map writing cancelled"));
	}

	exporter.reset();

	// Check for any stream failures now that we're done writing
	if (outFileStream.fail())
	{
		throw OperationException(fmt::format(_("Failure writing to file {0}"), outFile.string()));
	}

	if (auxFileStream && auxFileStream->fail())
	{
		throw OperationException(fmt::format(_("Failure writing to file {0}"), auxFile.string()));
	}
}

} // namespace map
