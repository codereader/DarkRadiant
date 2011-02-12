#include "Quake4MapFormat.h"

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
#include "string/string.h"

#include "primitiveparsers/BrushDef3.h"
#include "primitiveparsers/PatchDef2.h"
#include "primitiveparsers/PatchDef3.h"

#include <boost/lexical_cast.hpp>
#include "primitiveparsers/BrushDef.h"

#include "Doom3MapReader.h"
#include "Quake4MapWriter.h"

#include "Doom3MapFormat.h"

namespace map
{

	namespace
	{
		const std::string RKEY_FLOAT_PRECISION = "/mapFormat/floatPrecision";
	}

// RegisterableModule implementation
const std::string& Quake4MapFormat::getName() const
{
	static std::string _name("Quake4MapLoader");
	return _name;
}

const StringSet& Quake4MapFormat::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_FILETYPES);
		_dependencies.insert(MODULE_ECLASSMANAGER);
		_dependencies.insert(MODULE_LAYERSYSTEM);
		_dependencies.insert(MODULE_BRUSHCREATOR);
		_dependencies.insert(MODULE_PATCH + DEF2);
		_dependencies.insert(MODULE_PATCH + DEF3);
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_GAMEMANAGER);
		_dependencies.insert(MODULE_MAPFORMATMANAGER);
	}

	return _dependencies;
}

void Quake4MapFormat::initialiseModule(const ApplicationContext& ctx)
{
	globalOutputStream() << getName() << ": initialiseModule called." << std::endl;

	// Query game file - only register if the game type is q4
	if (GlobalGameManager().currentGame()->getKeyValue("type") == "quake4")
	{
		// Register ourselves as map format for maps and regions
		GlobalMapFormatManager().registerMapFormat("map", shared_from_this());
		GlobalMapFormatManager().registerMapFormat("reg", shared_from_this());

		// Add our primitive parsers to the global format registry
		GlobalMapFormatManager().registerPrimitiveParser(BrushDef3ParserQuake4Ptr(new BrushDef3ParserQuake4));
		GlobalMapFormatManager().registerPrimitiveParser(PatchDef2ParserPtr(new PatchDef2Parser));
		GlobalMapFormatManager().registerPrimitiveParser(PatchDef3ParserPtr(new PatchDef3Parser));
		GlobalMapFormatManager().registerPrimitiveParser(BrushDefParserPtr(new BrushDefParser));

		GlobalFiletypes().addType(
			"map", getName(), FileTypePattern(_("Quake 4 map"), "*.map"));
		GlobalFiletypes().addType(
			"map", getName(), FileTypePattern(_("Quake 4 region"), "*.reg"));
	}
}

void Quake4MapFormat::shutdownModule()
{
	if (GlobalGameManager().currentGame()->getKeyValue("type") == "quake4")
	{
		// Unregister now that we're shutting down
		GlobalMapFormatManager().unregisterMapFormat(shared_from_this());
	}
}

IMapReaderPtr Quake4MapFormat::getMapReader(IMapImportFilter& filter) const
{
	return IMapReaderPtr(new Doom3MapReader(filter));
}

IMapWriterPtr Quake4MapFormat::getMapWriter() const
{
	return IMapWriterPtr(new Quake4MapWriter);
}

bool Quake4MapFormat::allowInfoFileCreation() const
{
	// allow .darkradiant files to be saved
	return true;
}

bool Quake4MapFormat::canLoad(std::istream& stream) const
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

} // namespace map
