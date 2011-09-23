#include "MapResource.h"

#include "i18n.h"
#include <sstream>
#include <fstream>
#include <iostream>
#include "ifiletypes.h"
#include "igroupnode.h"
#include "ifilesystem.h"
#include "imainframe.h"
#include "iregistry.h"
#include "map/Map.h"
#include "map/RootNode.h"
#include "mapfile.h"
#include "gtkutil/dialog/MessageBox.h"
#include "referencecache/NullModelLoader.h"
#include "debugging/debugging.h"
#include "os/path.h"
#include "os/file.h"
#include "map/algorithm/Traverse.h"
#include "stream/textfilestream.h"
#include "referencecache/NullModelNode.h"

#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>

#include "InfoFile.h"
#include "string/string.h"

#include "algorithm/MapImporter.h"
#include "algorithm/MapExporter.h"
#include "algorithm/InfoFileExporter.h"
#include "algorithm/AssignLayerMappingWalker.h"
#include "algorithm/ChildPrimitives.h"

namespace fs = boost::filesystem;

namespace map
{

namespace
{
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
	_mapRoot(model::NullModelNode::InstancePtr()),
	_originalName(name),
	_type(os::getExtension(name)),
	_modified(0),
	_realised(false)
{
	// Initialise the paths, this is all needed for realisation
    _path = rootPath(_originalName);
	_name = os::getRelativePath(_originalName, _path);

	if (_infoFileExt.empty())
	{
		_infoFileExt = GlobalRegistry().get(RKEY_INFO_FILE_EXTENSION);
	}

	if (!_infoFileExt.empty() && _infoFileExt[0] != '.') 
	{
		_infoFileExt = "." + _infoFileExt;
	}
}

MapResource::~MapResource() {
    if (realised()) {
		unrealise();
	}
}

void MapResource::rename(const std::string& fullPath) {
	// Save the paths locally and split them into parts
	_originalName = fullPath;
	_type = fullPath.substr(fullPath.rfind(".") + 1);
	_path = rootPath(_originalName);
	_name = os::getRelativePath(_originalName, _path);

	// Rename the map root as well
	if (_mapRoot != NULL && boost::dynamic_pointer_cast<RootNode>(_mapRoot) != NULL) {
		boost::static_pointer_cast<RootNode>(_mapRoot)->setName(_name);
	}
}

bool MapResource::load() {
	ASSERT_MESSAGE(realised(), "resource not realised");
	if (_mapRoot == model::NullModelNode::InstancePtr()) {
		// Map not loaded yet, acquire map root node from loader
		_mapRoot = loadMapNode();

		connectMap();
		mapSave();
	}

	return _mapRoot != model::NullModelNode::InstancePtr();
}

/**
 * Save this resource (only for map resources).
 *
 * @returns
 * true if the resource was saved, false otherwise.
 */
bool MapResource::save()
{
	// For saving, take the default map format for this game type
	MapFormatPtr format = GlobalMapFormatManager().getMapFormatForGameType(
		GlobalGameManager().currentGame()->getKeyValue("type"), _type
	);

	if (format == NULL)
	{
		globalErrorStream() << "Could not locate map format module." << std::endl;
		return false;
	}
	
	std::string fullpath = _path + _name;

	// Save a backup of the existing file (rename it to .bak) if it exists in the first place
	if (file_exists(fullpath.c_str()))
	{
		if (!saveBackup())
		{
			// angua: if backup creation is not possible, still save the map
			// but create message in the console
			globalErrorStream() << "Could not create backup (Map is possibly open in Doom3)" << std::endl;
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
		globalErrorStream() << "Map path is not absolute: " << fullpath << std::endl;
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

		if (file_writeable(fullpath.string().c_str()))
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
				globalWarningStream() << "Error while creating backups: " << ex.what() << 
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
				globalWarningStream() << "Error while creating backups: " << ex.what() << 
					", the file is possibly opened by the game." << std::endl;
				errorOccurred = true;
			}

			return !errorOccurred;
		}
		else
		{
			globalErrorStream() << "map path is not writeable: " << fullpath.string() << std::endl;

			// File is write-protected
			gtkutil::MessageBox::ShowError(
				(boost::format(_("File is write-protected: %s")) % fullpath.string()).str(),
				GlobalMainFrame().getTopLevelWindow());

			return false;
		}
	}

	return false;
}

scene::INodePtr MapResource::getNode() {
	return _mapRoot;
}

void MapResource::setNode(scene::INodePtr node) {
	_mapRoot = node;
	connectMap();
}

void MapResource::addObserver(Observer& observer) {
	if (realised()) {
		observer.onResourceRealise();
	}
	_observers.insert(&observer);
}

void MapResource::removeObserver(Observer& observer) {
	if (realised()) {
		observer.onResourceUnrealise();
	}
	_observers.erase(&observer);
}

bool MapResource::realised() {
	return _realised;
}

// Realise this MapResource
void MapResource::realise() {
	if (_realised) {
		return; // nothing to do
	}

	_realised = true;

	// Realise the observers
	for (ResourceObserverList::iterator i = _observers.begin();
		 i != _observers.end(); i++)
	{
		(*i)->onResourceRealise();
	}
}

void MapResource::unrealise() {
	if (!_realised) {
		return; // nothing to do
	}

	_realised = false;

	// Realise the observers
	for (ResourceObserverList::iterator i = _observers.begin();
		 i != _observers.end(); i++)
	{
		(*i)->onResourceUnrealise();
	}

	//globalOutputStream() << "MapResource::unrealise: " << _path.c_str() << _name.c_str() << "\n";
	_mapRoot = model::NullModelNode::InstancePtr();
}

void MapResource::onMapChanged() {
	GlobalMap().setModified(true);
}

void MapResource::connectMap() {
    MapFilePtr map = Node_getMapFile(_mapRoot);
    if (map != NULL) {
    	// Reroute the changed callback to the onMapChanged() call.
		map->setChangedCallback(boost::bind(&MapResource::onMapChanged, this));
    }
}

std::time_t MapResource::modified() const {
	std::string fullpath = _path + _name;
	return file_modified(fullpath.c_str());
}

void MapResource::mapSave() {
	_modified = modified();
	MapFilePtr map = Node_getMapFile(_mapRoot);
	if (map != NULL) {
		map->save();
	}
}

bool MapResource::isModified() const {
	// had or has an absolute path // AND disk timestamp changed
	return (!_path.empty() && _modified != modified())
			|| !path_equal(rootPath(_originalName).c_str(), _path.c_str()); // OR absolute vfs-root changed
}

void MapResource::reload()
{
    unrealise();
	realise();
}

MapFormatPtr MapResource::determineMapFormat(std::istream& stream)
{
	// Get all registered map formats
	std::set<MapFormatPtr> availableFormats = GlobalMapFormatManager().getMapFormatList(_type);

	MapFormatPtr format;

	for (std::set<MapFormatPtr>::const_iterator f = availableFormats.begin(); f != availableFormats.end(); ++f)
	{
		// Rewind the stream before passing it to the format for testing
		// Map format valid, rewind the stream
		stream.seekg(0, std::ios_base::beg);

		if ((*f)->canLoad(stream))
		{
			format = *f;
			break;
		}
	}

	// Rewind the stream when we're done
	stream.seekg(0, std::ios_base::beg);

	return format;
}

scene::INodePtr MapResource::loadMapNode()
{
	// greebo: Check if we have valid settings
	// The _path might be empty if we're loading from a folder outside the mod
	if (_name.empty() && _type.empty())
	{
		return model::NullModelNode::InstancePtr();
	}

	// Build the map path
	std::string fullpath = _path + _name;

	if (path_is_absolute(fullpath.c_str()))
	{
		globalOutputStream() << "Open file " << fullpath << " for determining the map format...";

		TextFileInputStream file(fullpath);
		std::istream mapStream(&file);

		if (file.failed())
		{
			globalErrorStream() << "failure" << std::endl;

			gtkutil::MessageBox::ShowError(
				(boost::format(_("Failure opening map file:\n%s")) % fullpath).str(),
				GlobalMainFrame().getTopLevelWindow());

			return model::NullModelNode::InstancePtr();
		}

		globalOutputStream() << "success" << std::endl;

		// Get the mapformat
		MapFormatPtr format = determineMapFormat(mapStream);

		if (format == NULL)
		{
			gtkutil::MessageBox::ShowError(
				(boost::format(_("Could not determine map format of file:\n%s")) % fullpath).str(),
				GlobalMainFrame().getTopLevelWindow());

			return model::NullModelNode::InstancePtr();
		}
		
		// Map format valid, rewind the stream
		mapStream.seekg(0, std::ios_base::beg);

		// Create a new map root node
		scene::INodePtr root(NewMapRoot(_name));

		if (loadFile(mapStream, *format, root, fullpath))
		{
			return root;
		}
	}
	else 
	{
		globalErrorStream() << "map path is not fully qualified: " << fullpath << std::endl;
	}

	// Return the NULL node on failure
	return model::NullModelNode::InstancePtr();
}

bool MapResource::loadFile(std::istream& mapStream, const MapFormat& format, const scene::INodePtr& root, const std::string& filename)
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
		std::string infoFilename(filename.substr(0, filename.rfind('.')));
		infoFilename += GlobalRegistry().get(RKEY_INFO_FILE_EXTENSION);

		std::ifstream infoFileStream(infoFilename.c_str());

		if (infoFileStream.is_open())
		{
			globalOutputStream() << " found information file... ";
		}

		globalOutputStream() << "success" << std::endl;

		// Read the infofile
		InfoFile infoFile(infoFileStream);

		try
		{
			// Start parsing, this will throw if any errors occur
			infoFile.parse();

			// Create the layers according to the data found in the map information file
			const InfoFile::LayerNameMap& layers = infoFile.getLayerNames();

			for (InfoFile::LayerNameMap::const_iterator i = layers.begin();
				 i != layers.end(); ++i)
			{
				// Create the named layer with the saved ID
				GlobalLayerSystem().createLayer(i->second, i->first);
			}

			// Now that the graph is in place, assign the layers
			AssignLayerMappingWalker walker(infoFile);
			root->traverse(walker);
		}
		catch (parser::ParseException& e)
		{
			globalErrorStream() << "[MapResource] Unable to parse info file: " << e.what() << std::endl;
		}

		return true;
	}
	catch (gtkutil::ModalProgressDialog::OperationAbortedException&)
	{
		gtkutil::MessageBox::ShowError(
			_("Map loading cancelled"),
			GlobalMainFrame().getTopLevelWindow()
		);

		// Clear out the root node, otherwise we end up with half a map
		scene::NodeRemover remover;
		root->traverse(remover);

		return false;
	}
	catch (IMapReader::FailureException& e)
	{
		gtkutil::MessageBox::ShowError(
				(boost::format(_("Failure reading map file:\n%s\n\n%s")) % filename % e.what()).str(),
				GlobalMainFrame().getTopLevelWindow());

		// Clear out the root node, otherwise we end up with half a map
		scene::NodeRemover remover;
		root->traverse(remover);

		return false;
	}
}

std::string MapResource::getTemporaryFileExtension()
{
	time_t localtime;
	time(&localtime);

	return sizetToStr(localtime);
}

bool MapResource::checkIsWriteable(const boost::filesystem::path& path)
{
	// Check writeability of the given file
	if (boost::filesystem::exists(path) && !file_writeable(path.string().c_str()))
	{
		// File is write-protected
		globalErrorStream() << "File is write-protected." << std::endl;

		gtkutil::MessageBox::ShowError(
			(boost::format(_("File is write-protected: %s")) % path.string()).str(),
			GlobalMainFrame().getTopLevelWindow());

		return false;
	}

	return true;
}

bool MapResource::saveFile(const MapFormat& format, const scene::INodePtr& root,
						   GraphTraversalFunc traverse, const std::string& filename)
{
	// greebo: When auto-saving large maps users occasionally hit cancel
	// we need to make sure to write to temporary files so that we don't
	// leave a corrupt .map and .darkradiant file behind.
	std::string tempExt = getTemporaryFileExtension();

	// Actual output file paths
	fs::path outFile = filename;
	fs::path auxFile = outFile;
	auxFile.replace_extension(_infoFileExt);

	// Temporary file paths
	fs::path tempOutFile = outFile;
	tempOutFile.replace_extension(".map" + tempExt);

	fs::path tempAuxFile = auxFile;
	tempAuxFile.replace_extension(".aux" + tempExt);

	// Check writeability of the output files
	if (!checkIsWriteable(outFile)) return false;
	if (!checkIsWriteable(auxFile)) return false;
	if (!checkIsWriteable(tempOutFile)) return false;
	if (!checkIsWriteable(tempAuxFile)) return false;

	// Test opening the output file
	globalOutputStream() << "Opening file " << tempOutFile.string() << " ";
	
	// Open the stream to the output file
	std::ofstream outFileStream(tempOutFile.string().c_str());

	globalOutputStream() << "and auxiliary file " << tempAuxFile.string() << " for writing...";

	std::ofstream auxFileStream(tempAuxFile.string().c_str());

	if (outFileStream.is_open() && auxFileStream.is_open())
	{
		globalOutputStream() << "success" << std::endl;

		// Check the total count of nodes to traverse
		NodeCounter counter;
		traverse(root, counter);
		
		// Acquire the MapWriter from the MapFormat class
		IMapWriterPtr mapWriter = format.getMapWriter();

		// Create our main MapExporter walker, and pass the desired 
		// writer to it. The constructor will prepare the scene
		// and the destructor will clean it up afterwards. That way
		// we ensure a nice and tidy scene when exceptions are thrown.
		MapExporter exporter(*mapWriter, root, outFileStream, counter.getCount());

		bool cancelled = false;

		try
		{
			// Use the traversal function to start pushing relevant nodes
			// to the MapExporter
			traverse(root, exporter);

			// Now traverse the scene again and write the .darkradiant file,
			// provided the MapFormat doesn't disallow layer saving.
			if (format.allowInfoFileCreation())
			{
				InfoFileExporter infoExporter(root, auxFileStream);
				traverse(root, infoExporter);
			}
		}
		catch (gtkutil::ModalProgressDialog::OperationAbortedException&)
		{
			gtkutil::MessageBox::ShowError(
				_("Map writing cancelled"),
				GlobalMainFrame().getTopLevelWindow()
			);

			cancelled = true;
		}

		outFileStream.close();
		auxFileStream.close();

		// If the user cancelled the operation, just remove the temporary files 
		if (cancelled)
		{
			fs::remove(tempOutFile);
			fs::remove(tempAuxFile);

			return true;
		}

		// Move the temporary files over to the target path
		try
		{
			if (fs::exists(outFile)) 
			{
				fs::remove(outFile);
			}
			fs::rename(tempOutFile, outFile);

			if (fs::exists(auxFile)) 
			{
				fs::remove(auxFile);
			}
			fs::rename(tempAuxFile, auxFile);
		}
		catch (fs::filesystem_error& ex)
		{
			globalErrorStream() << "Error moving temporary files to destination paths: "
				<< ex.what() << std::endl;
		}

	    return true;
	}
	else
	{
		globalErrorStream() << "failure" << std::endl;
		return false;
	}
}

} // namespace map
