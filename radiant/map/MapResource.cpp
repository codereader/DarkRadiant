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
#include "gamelib.h"
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
#include "scene/LayerValidityCheckWalker.h"

namespace fs = boost::filesystem;

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
		_infoFileExt = game::current::getValue<std::string>(GKEY_INFO_FILE_EXTENSION);
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
    RootNodePtr root = boost::dynamic_pointer_cast<RootNode>(_mapRoot);
	if (root)
    {
		root->setName(_name);
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

	//rMessage() << "MapResource::unrealise: " << _path.c_str() << _name.c_str() << "\n";
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
		rMessage() << "Open file " << fullpath << " for determining the map format...";

		TextFileInputStream file(fullpath);
		std::istream mapStream(&file);

		if (file.failed())
		{
			rError() << "failure" << std::endl;

			gtkutil::MessageBox::ShowError(
				(boost::format(_("Failure opening map file:\n%s")) % fullpath).str(),
				GlobalMainFrame().getTopLevelWindow());

			return model::NullModelNode::InstancePtr();
		}

		rMessage() << "success" << std::endl;

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
		rError() << "map path is not fully qualified: " << fullpath << std::endl;
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
		infoFilename += game::current::getValue<std::string>(GKEY_INFO_FILE_EXTENSION);

		std::ifstream infoFileStream(infoFilename.c_str());

		if (infoFileStream.is_open())
		{
			rMessage() << " found information file... ";
		}

		rMessage() << "success" << std::endl;

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
			root->traverseChildren(walker);

			rMessage() << "Sanity-checking the layer assignments...";

			// Sanity-check the layer mapping, it's possible that some .darkradiant
			// files are mapping nodes to non-existent layer IDs
			scene::LayerValidityCheckWalker checker;
			root->traverseChildren(checker);

			rMessage() << "done, had to fix " << checker.getNumFixed() << " assignments." << std::endl;

			// Remove all selection sets, there shouldn't be many left at this point
			GlobalSelectionSetManager().deleteAllSelectionSets();

			// Re-construct the selection sets
			infoFile.foreachSelectionSetInfo([&] (const InfoFile::SelectionSetImportInfo& info)
			{
				selection::ISelectionSetPtr set = GlobalSelectionSetManager().createSelectionSet(info.name);

				std::size_t failedNodes = 0;

				std::for_each(info.nodeIndices.begin(), info.nodeIndices.end(), 
					[&] (const InfoFile::SelectionSetImportInfo::IndexPair& indexPair)
				{
					scene::INodePtr node = importFilter.getNodeByIndexPair(indexPair);

					if (node)
					{
						set->addNode(node);
					}
					else
					{
						failedNodes++;
					}
				});

				if (failedNodes > 0)
				{
					rWarning() << "Couldn't resolve " << failedNodes << " nodes in selection set " << set->getName() << std::endl;
				}
			});
		}
		catch (parser::ParseException& e)
		{
			rError() << "[MapResource] Unable to parse info file: " << e.what() << std::endl;
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
		root->traverseChildren(remover);

		return false;
	}
	catch (IMapReader::FailureException& e)
	{
		gtkutil::MessageBox::ShowError(
				(boost::format(_("Failure reading map file:\n%s\n\n%s")) % filename % e.what()).str(),
				GlobalMainFrame().getTopLevelWindow());

		// Clear out the root node, otherwise we end up with half a map
		scene::NodeRemover remover;
		root->traverseChildren(remover);

		return false;
	}
}

std::string MapResource::getTemporaryFileExtension()
{
	time_t localtime;
	time(&localtime);

	return string::to_string(localtime);
}

bool MapResource::checkIsWriteable(const boost::filesystem::path& path)
{
	// Check writeability of the given file
	if (boost::filesystem::exists(path) && !file_writeable(path.string().c_str()))
	{
		// File is write-protected
		rError() << "File is write-protected." << std::endl;

		gtkutil::MessageBox::ShowError(
			(boost::format(_("File is write-protected: %s")) % path.string()).str(),
			GlobalMainFrame().getTopLevelWindow());

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

	// Check writeability of the output files
	if (!checkIsWriteable(outFile)) return false;
	if (!checkIsWriteable(auxFile)) return false;

	// Test opening the output file
	rMessage() << "Opening file " << outFile.string() << " ";
	
	// Open the stream to the output file
	std::ofstream outFileStream(outFile.string().c_str());

	rMessage() << "and auxiliary file " << auxFile.string() << " for writing...";

	std::ofstream auxFileStream(auxFile.string().c_str());

	if (outFileStream.is_open() && auxFileStream.is_open())
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
			exporter.reset(new MapExporter(*mapWriter, root, outFileStream, auxFileStream, counter.getCount()));
		}
		else
		{
			exporter.reset(new MapExporter(*mapWriter, root, outFileStream, counter.getCount())); // no aux stream
		}

		bool cancelled = false;

		try
		{
			// Pass the traversal function and the root of the subgraph to export
			exporter->exportMap(root, traverse);
		}
		catch (gtkutil::ModalProgressDialog::OperationAbortedException&)
		{
			gtkutil::MessageBox::ShowError(
				_("Map writing cancelled"),
				GlobalMainFrame().getTopLevelWindow()
			);

			cancelled = true;
		}

		exporter.reset();

		outFileStream.close();
		auxFileStream.close();

		return !cancelled;
	}
	else
	{
		gtkutil::MessageBox::ShowError(
			_("Could not open output streams for writing"),
			GlobalMainFrame().getTopLevelWindow()
		);

		rError() << "failure" << std::endl;
		return false;
	}
}

} // namespace map
