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
#include "gtkutil/dialog.h"
#include "referencecache/NullModelLoader.h"
#include "debugging/debugging.h"
#include "os/path.h"
#include "os/file.h"
#include "map/algorithm/Traverse.h"
#include "stream/textfilestream.h"
#include "referencecache/NullModelNode.h"
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include "InfoFile.h"

#include "algorithm/MapImporter.h"
#include "algorithm/MapExporter.h"
#include "algorithm/InfoFileExporter.h"
#include "algorithm/AssignLayerMappingWalker.h"
#include "algorithm/ChildPrimitives.h"

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

// Constructor
MapResource::MapResource(const std::string& name) :
	_mapRoot(model::NullModelNode::InstancePtr()),
	_originalName(name),
	_type(name.substr(name.rfind(".") + 1)),
	_modified(0),
	_realised(false)
{
	// Initialise the paths, this is all needed for realisation
    _path = rootPath(_originalName);
	_name = os::getRelativePath(_originalName, _path);
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
bool MapResource::save() {
	std::string moduleName = GlobalFiletypes().findModuleName("map", _type);

	if (!moduleName.empty()) {
		MapFormatPtr format = boost::dynamic_pointer_cast<MapFormat>(
			module::GlobalModuleRegistry().getModule(moduleName)
		);

		if (format == NULL) {
			globalErrorStream() << "Could not locate map loader module." << std::endl;
			return false;
		}

		std::string fullpath = _path + _name;

		// Save a backup of the existing file (rename it to .bak) if it exists in the first place
		if (file_exists(fullpath.c_str())) {
			if (!saveBackup()) {
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
		else {
			globalErrorStream() << "Map path is not absolute: " << fullpath << std::endl;
			success = false;
		}

		if (success) {
  			mapSave();
  			return true;
		}
	}

	return false;
}

bool MapResource::saveBackup() {
	std::string fullpath = _path + _name;

	if (path_is_absolute(fullpath.c_str())) {
		// Save a backup if possible. This is done by renaming the original,
		// which won't work if the existing map is currently open by Doom 3
		// in the background.
		if (!file_exists(fullpath.c_str())) {
			return false;
		}

		if (file_writeable(fullpath.c_str())) {
			std::string pathWithoutExtension = fullpath.substr(0, fullpath.rfind('.'));
			std::string backup = pathWithoutExtension + ".bak";

			return (!file_exists(backup.c_str()) || file_remove(backup.c_str())) // remove backup
				&& file_move(fullpath.c_str(), backup.c_str()); // rename current to backup
		}
		else {
			globalErrorStream() << "map path is not writeable: " << fullpath << std::endl;
			// File is write-protected
			gtkutil::errorDialog(
				(boost::format(_("File is write-protected: %s")) % fullpath).str(),
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

	for (std::set<MapFormatPtr>::const_iterator f = availableFormats.begin(); f != availableFormats.end(); ++f)
	{
		// Rewind the stream before passing it to the format for testing
		// Map format valid, rewind the stream
		stream.seekg(0, std::ios_base::beg);

		if ((*f)->canLoad(stream))
		{
			return *f;
		}
	}

	return MapFormatPtr();
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

			gtkutil::errorDialog(
				(boost::format(_("Failure opening map file:\n%s")) % fullpath).str(),
				GlobalMainFrame().getTopLevelWindow());

			return model::NullModelNode::InstancePtr();
		}

		globalOutputStream() << "success" << std::endl;

		// Get the mapformat
		MapFormatPtr format = determineMapFormat(mapStream);

		if (format == NULL)
		{
			gtkutil::errorDialog(
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
	catch (gtkutil::ModalProgressDialog::OperationAbortedException& p)
	{
		gtkutil::errorDialog(
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
		gtkutil::errorDialog(
				(boost::format(_("Failure reading map file:\n%s\n\n%s")) % filename % e.what()).str(),
				GlobalMainFrame().getTopLevelWindow());

		// Clear out the root node, otherwise we end up with half a map
		scene::NodeRemover remover;
		root->traverse(remover);

		return false;
	}
}

bool MapResource::saveFile(const MapFormat& format, const scene::INodePtr& root,
						   GraphTraversalFunc traverse, const std::string& filename)
{
	globalOutputStream() << "Open file " << filename << " ";

	if (file_exists(filename.c_str()) && !file_writeable(filename.c_str()))
	{
		// File is write-protected
		globalErrorStream() << "failure, file is write-protected." << std::endl;
		gtkutil::errorDialog(
			(boost::format(_("File is write-protected: %s")) % filename).str(),
			GlobalMainFrame().getTopLevelWindow());
		return false;
	}

	// Open the stream to the output file
	std::ofstream outfile(filename.c_str());

	// Open the auxiliary file too
	std::string auxFilename(filename);
	auxFilename = auxFilename.substr(0, auxFilename.rfind('.'));
	auxFilename += GlobalRegistry().get(RKEY_INFO_FILE_EXTENSION);

	globalOutputStream() << "and auxiliary file " << auxFilename << " for writing...";

	if (file_exists(auxFilename.c_str()) && !file_writeable(auxFilename.c_str())) {
		// File is write-protected
		globalErrorStream() << "failure, file is write-protected." << std::endl;
		gtkutil::errorDialog(
			(boost::format(_("File is write-protected: %s")) % auxFilename).str(),
			GlobalMainFrame().getTopLevelWindow());
		return false;
	}

	std::ofstream auxfile(auxFilename.c_str());

	if (outfile.is_open() && auxfile.is_open())
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
		MapExporter exporter(*mapWriter, root, outfile, counter.getCount());

		try
		{
			// Use the traversal function to start pushing relevant nodes
			// to the MapExporter
			traverse(root, exporter);

			// Now traverse the scene again and write the .darkradiant file,
			// provided the MapFormat doesn't disallow layer saving.
			if (format.allowInfoFileCreation())
			{
				InfoFileExporter infoExporter(root, auxfile);
				traverse(root, infoExporter);
			}
		}
		catch (gtkutil::ModalProgressDialog::OperationAbortedException&)
		{
			gtkutil::errorDialog(
				_("Map writing cancelled"),
				GlobalMainFrame().getTopLevelWindow()
			);
		}

		outfile.close();
		auxfile.close();
	    return true;
	}
	else {
		globalErrorStream() << "failure" << std::endl;
		return false;
	}
}

} // namespace map
