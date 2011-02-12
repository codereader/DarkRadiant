#include "Doom3MapFormat.h"

#include "itextstream.h"
#include "ifiletypes.h"
#include "ieclass.h"
#include "ibrush.h"
#include "ipatch.h"
#include "igame.h"
#include "iregistry.h"
#include "igroupnode.h"

#include "parser/DefTokeniser.h"

#include "scenelib.h"

#include "i18n.h"
#include "Tokens.h"
#include "MapImportInfo.h"
#include "string/string.h"

#include "primitiveparsers/BrushDef3.h"
#include "primitiveparsers/PatchDef2.h"
#include "primitiveparsers/PatchDef3.h"

#include <boost/lexical_cast.hpp>
#include "primitiveparsers/BrushDef.h"

#include "Doom3MapReader.h"
#include "Doom3MapWriter.h"

namespace map {

	namespace {
		const std::string RKEY_FLOAT_PRECISION = "/mapFormat/floatPrecision";
	}

// RegisterableModule implementation
const std::string& Doom3MapFormat::getName() const {
	static std::string _name("Doom3MapLoader");
	return _name;
}

const StringSet& Doom3MapFormat::getDependencies() const {
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_FILETYPES);
		_dependencies.insert(MODULE_ECLASSMANAGER);
		_dependencies.insert(MODULE_LAYERSYSTEM);
		_dependencies.insert(MODULE_BRUSHCREATOR);
		_dependencies.insert(MODULE_PATCH + DEF2);
		_dependencies.insert(MODULE_PATCH + DEF3);
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_MAPFORMATMANAGER);
	}

	return _dependencies;
}

void Doom3MapFormat::initialiseModule(const ApplicationContext& ctx)
{
	globalOutputStream() << getName() << ": initialiseModule called." << std::endl;

	// Register ourselves as map format for maps and regions
	GlobalMapFormatManager().registerMapFormat("map", shared_from_this());
	GlobalMapFormatManager().registerMapFormat("reg", shared_from_this());

	// Add our primitive parsers to the global format registry
	GlobalMapFormatManager().registerPrimitiveParser(BrushDef3ParserPtr(new BrushDef3Parser));
	GlobalMapFormatManager().registerPrimitiveParser(PatchDef2ParserPtr(new PatchDef2Parser));
	GlobalMapFormatManager().registerPrimitiveParser(PatchDef3ParserPtr(new PatchDef3Parser));
	GlobalMapFormatManager().registerPrimitiveParser(BrushDefParserPtr(new BrushDefParser));

	GlobalFiletypes().addType(
		"map", getName(), FileTypePattern(_("Doom 3 map"), "*.map"));
	GlobalFiletypes().addType(
		"map", getName(), FileTypePattern(_("Doom 3 region"), "*.reg"));
}

void Doom3MapFormat::shutdownModule()
{
	// Unregister now that we're shutting down
	GlobalMapFormatManager().unregisterMapFormat(shared_from_this());
}

IMapReaderPtr Doom3MapFormat::getMapReader(IMapImportFilter& filter) const
{
	return IMapReaderPtr(new Doom3MapReader(filter));
}

IMapWriterPtr Doom3MapFormat::getMapWriter() const
{
	return IMapWriterPtr(new Doom3MapWriter);
}

bool Doom3MapFormat::allowInfoFileCreation() const
{
	// allow .darkradiant files to be saved
	return true;
}

bool Doom3MapFormat::canLoad(std::istream& stream) const
{
	// Load the required version from the .game file
	xml::NodeList nodes = GlobalGameManager().currentGame()->getLocalXPath(RKEY_GAME_MAP_VERSION);
	assert(!nodes.empty());

	float requiredVersion = strToFloat(nodes[0].getAttributeValue("value"));

	// Instantiate a tokeniser to read the first few tokens
	parser::BasicDefTokeniser<std::istream> tok(stream);

	try
	{
		// Require a "Version" token
		tok.assertNextToken(VERSION);

		// Require specific version, return true on success 
		return (boost::lexical_cast<float>(tok.nextToken()) == requiredVersion);
	}
	catch (parser::ParseException&)
	{}
	catch (boost::bad_lexical_cast&)
	{}

	return false;
}

#if 0
void Doom3MapFormat::readGraph(const MapImportInfo& importInfo) const
{
	assert(importInfo.root != NULL);

	// Construct a MapImporter that will do the map parsing
	Doom3MapReader importer(importInfo);

	if (importer.parse())
	{
		// Run the post-process (layers and/or child primitive origins)
		onMapParsed(importInfo);

		return;
	}
	else
	{
		// Importer return FALSE, propagate this failure
		throw FailureException("General parser failure.");
	}
}

void Doom3MapFormat::addOriginToChildPrimitives(const scene::INodePtr& root) const
{
	// Disable texture lock during this process
	bool textureLockStatus = GlobalRegistry().get(RKEY_ENABLE_TEXTURE_LOCK) == "1";
	GlobalRegistry().set(RKEY_ENABLE_TEXTURE_LOCK, "0");

	// Local helper to add origins
	class OriginAdder :
		public scene::NodeVisitor
	{
	public:
		// NodeVisitor implementation
		bool pre(const scene::INodePtr& node) {
			Entity* entity = Node_getEntity(node);

			// Check for an entity
			if (entity != NULL) {
				// greebo: Check for a Doom3Group
				scene::GroupNodePtr groupNode = Node_getGroupNode(node);

				// Don't handle the worldspawn children, they're safe&sound
				if (groupNode != NULL && entity->getKeyValue("classname") != "worldspawn") {
					groupNode->addOriginToChildren();
					// Don't traverse the children
					return false;
				}
			}
			return true;
		}
	} adder;

	Node_traverseSubgraph(root, adder);

	GlobalRegistry().set(RKEY_ENABLE_TEXTURE_LOCK, textureLockStatus ? "1" : "0");
}

void Doom3MapFormat::removeOriginFromChildPrimitives(const scene::INodePtr& root) const
{
	// Disable texture lock during this process
	bool textureLockStatus = GlobalRegistry().get(RKEY_ENABLE_TEXTURE_LOCK) == "1";
	GlobalRegistry().set(RKEY_ENABLE_TEXTURE_LOCK, "0");

	// Local helper to remove the origins
	class OriginRemover :
		public scene::NodeVisitor
	{
	public:
		bool pre(const scene::INodePtr& node) {
			Entity* entity = Node_getEntity(node);

			// Check for an entity
			if (entity != NULL) {
				// greebo: Check for a Doom3Group
				scene::GroupNodePtr groupNode = Node_getGroupNode(node);

				// Don't handle the worldspawn children, they're safe&sound
				if (groupNode != NULL && entity->getKeyValue("classname") != "worldspawn") {
					groupNode->removeOriginFromChildren();
					// Don't traverse the children
					return false;
				}
			}

			return true;
		}
	} remover;

	Node_traverseSubgraph(root, remover);

	GlobalRegistry().set(RKEY_ENABLE_TEXTURE_LOCK, textureLockStatus ? "1" : "0");
}

void Doom3MapFormat::onMapParsed(const MapImportInfo& importInfo) const
{
	/*// Read the infofile
	InfoFile infoFile(importInfo.infoStream);

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
		importInfo.root->traverse(walker);
	}
	catch (parser::ParseException& e)
	{
		globalErrorStream() << "[mapdoom3] Unable to parse info file: " << e.what() << std::endl;
    }*/

	// Also process the func_static child primitives
	addOriginToChildPrimitives(importInfo.root);
}
#endif

} // namespace map
